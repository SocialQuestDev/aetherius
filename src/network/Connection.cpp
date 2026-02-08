#include "../../include/network/Connection.h"
#include "../../include/Logger.h"
#include "../../include/game/world/Chunk.h"
#include "../../include/Server.h"
#include "../../include/crypto/RSA.h"
#include "../../include/auth/MojangAuthHelper.h"
#include "../../include/utility/VectorUtilities.h"
#include "../../include/game/player/PlayerList.h"
#include "../../include/game/player/Player.h"
#include "../../include/game/world/World.h"

#include <toml++/toml.hpp>
#include <nlohmann/json.hpp>
#include <zlib.h>
#include <fstream>

using json = nlohmann::json;

Connection::Connection(boost::asio::io_context& io_context) : socket_(io_context), buffer_{} {}

std::shared_ptr<Connection> Connection::create(boost::asio::io_context& io_context) {
    return std::shared_ptr<Connection>(new Connection(io_context));
}

tcp::socket& Connection::socket() { return socket_; }

void Connection::start() { do_read(); }

static std::string base64_encode_local(const std::vector<unsigned char>& data) {
    static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (const unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

static std::string getIconBase64(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    const std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return "data:image/png;base64," + base64_encode_local(buffer);
}

void Connection::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(buffer_),
        [this, self](const boost::system::error_code &ec, const std::size_t length) {
            if (!ec) {
                std::vector<uint8_t> received(buffer_, buffer_ + length);
                if (encrypt && crypto_state) aes::decrypt(*crypto_state, received.data(), (int)received.size());
                try { handle_packet(received); }
                catch (std::exception& e) { LOG_ERROR("Packet parsing error: " + std::string(e.what())); }
                do_read();
            } else { LOG_DEBUG("Connection closed: " + ec.message()); }
        });
}

void Connection::send_join_game() {
    PacketBuffer joinGame;
    joinGame.writeVarInt(0x24);
    joinGame.writeInt(1); 
    joinGame.writeBoolean(false); 
    joinGame.writeByte(1); 
    joinGame.writeByte(255); 
    joinGame.writeVarInt(1); 
    joinGame.writeString("minecraft:overworld");
    World& world = Server::get_instance().get_world();
    joinGame.writeNbt(world.getDimensionCodec());
    joinGame.writeNbt(world.getDimension());
    joinGame.writeString("minecraft:overworld");
    joinGame.writeLong(0); 
    joinGame.writeVarInt(20); 
    joinGame.writeVarInt(10); 
    joinGame.writeBoolean(false); 
    joinGame.writeBoolean(true); 
    joinGame.writeBoolean(false); 
    joinGame.writeBoolean(false); 
    send_packet(joinGame);

    // Пакет способностей (0x32 для 1.16.5) - КРИТИЧНО для движения
    /*PacketBuffer abilities;
    abilities.writeVarInt(0x30); // Твой ID
    abilities.writeByte(0x00);   // Флаги: 0x02 - можно летать, 0x00 - просто стоять на земле
    abilities.writeFloat(0.05f); // Скорость полета
    abilities.writeFloat(0.1f);  // Скорость ходьбы
    send_packet(abilities);*/

    // Позиция: Y=7.0 (блоки на 5.0)
    PacketBuffer posLook;
    posLook.writeVarInt(0x34); 
    posLook.writeDouble(0.0);  
    posLook.writeDouble(7.0); 
    posLook.writeDouble(0.0);  
    posLook.writeFloat(0.0f);  
    posLook.writeFloat(0.0f);  
    posLook.writeByte(0x00);   
    posLook.writeVarInt(1);    // Teleport ID
    send_packet(posLook);
}

void Connection::send_light_data(int chunkX, int chunkZ) {
    PacketBuffer lightPacket;
    lightPacket.writeVarInt(0x23); // Твой ID
    lightPacket.writeVarInt(chunkX);
    lightPacket.writeVarInt(chunkZ);
    // УБРАЛИ writeBoolean(true) — в 1.16.5 его тут нет!
    
    lightPacket.writeVarInt(0x3FFFF); // Sky Mask
    lightPacket.writeVarInt(0x3FFFF); // Block Mask
    lightPacket.writeVarInt(0);       // Empty Sky Mask
    lightPacket.writeVarInt(0);       // Empty Block Mask

    // Данные секций... (оставляем как было)
    for (int i = 0; i < 36; ++i) { 
        lightPacket.writeVarInt(2048);
        for (int j = 0; j < 2048; ++j) lightPacket.writeByte(0xFF); 
    }
    send_packet(lightPacket);
}

void Connection::handle_packet(std::vector<uint8_t>& rawData) {
    PacketBuffer raw(rawData);
    try {
        Server& server = Server::get_instance();
        const toml::table& serverCfg = server.get_config();

        while (raw.readerIndex < raw.data.size()) {
            int packetLength = raw.readVarInt();
            if (packetLength < 1) break;

            std::vector<uint8_t> payload;
            if (compression_enabled) {
                size_t startIdx = raw.readerIndex;
                int dataLength = raw.readVarInt();
                int remainingBytes = packetLength - (int)(raw.readerIndex - startIdx);
                if (dataLength > 0) {
                    payload.resize(dataLength);
                    unsigned long destLen = dataLength;
                    uncompress(payload.data(), &destLen, raw.data.data() + raw.readerIndex, remainingBytes);
                    raw.readerIndex += remainingBytes;
                } else {
                    payload.assign(raw.data.begin() + raw.readerIndex, raw.data.begin() + raw.readerIndex + remainingBytes);
                    raw.readerIndex += remainingBytes;
                }
            } else {
                payload.assign(raw.data.begin() + raw.readerIndex, raw.data.begin() + raw.readerIndex + packetLength);
                raw.readerIndex += packetLength;
            }

            PacketBuffer reader(payload);
            int packetID = reader.readVarInt();

            if (state_ == State::HANDSHAKE && packetID == 0x00) {
                reader.readVarInt(); reader.readString(); reader.readUShort();
                int next = reader.readVarInt();
                state_ = (next == 1) ? State::STATUS : State::LOGIN;
            }
            else if (state_ == State::STATUS) {
                if (packetID == 0x00) {
                    json resp;
                    resp["version"] = {{"name", "1.16.5"}, {"protocol", 754}};
                    resp["players"] = {{"max", 20}, {"online", 0}, {"sample", json::array()}};
                    resp["description"]["text"] = "Aetherius Server";
                    PacketBuffer res; res.writeVarInt(0x00); res.writeString(resp.dump());
                    send_packet(res);
                } else if (packetID == 0x01) {
                    PacketBuffer pong; pong.writeVarInt(0x01); pong.writeLong(reader.readLong());
                    send_packet(pong);
                }
            }
            else if (state_ == State::LOGIN && packetID == 0x00) {
                nickname = reader.readString();
                if (serverCfg["server"]["compression_enabled"].value_or(false)) {
                    int th = serverCfg["server"]["compression_threshold"].value_or(256);
                    PacketBuffer comp; comp.writeVarInt(0x03); comp.writeVarInt(th);
                    send_packet_raw(comp.finalize(false, -1, nullptr));
                    compression_enabled = true;
                }
                PacketBuffer succ; succ.writeVarInt(0x02); succ.writeUUID(get_offline_UUID_128(nickname));
                succ.writeString(nickname); send_packet(succ);
                state_ = State::PLAY; send_join_game();
            } 
            else if (state_ == State::PLAY) {
                if (packetID == 0x00) { // Teleport Confirm
                    World& world = server.get_world();
                    for (int x = -1; x <= 1; ++x) {
                        for (int z = -1; z <= 1; ++z) {
                            ChunkColumn* chunk = world.generateChunk(x, z);
                            PacketBuffer cp; cp.writeVarInt(0x20);
                            auto p = chunk->serialize();
                            cp.data.insert(cp.data.end(), p.begin(), p.end());
                            send_packet(cp); //send_light_data(x, z);
                        }
                    }
                    PacketBuffer vp; vp.writeVarInt(0x40); vp.writeVarInt(0); vp.writeVarInt(0);
                    send_packet(vp);
                } 
                else if (packetID >= 0x11 && packetID <= 0x14) {
                    // Читаем данные движения, чтобы не "лагало"
                    if (packetID == 0x11 || packetID == 0x12) { 
                        reader.readDouble(); reader.readDouble(); reader.readDouble(); 
                    }
                    if (packetID == 0x12 || packetID == 0x13) { 
                        reader.readFloat(); reader.readFloat(); 
                    }
                    reader.readBoolean(); // onGround
                }
                else if (packetID >= 0x11 && packetID <= 0x14) {
                    // Просто переставляем индекс чтения в конец этого конкретного пакета (payload)
                    // Это гарантирует, что мы не будем читать лишнего и не оставим данных
                    reader.readerIndex = payload.size(); 
                }
                else if (packetID == 0x10) {
                    uint64_t id = reader.readLong();
                    PacketBuffer ka;
                    ka.writeVarInt(0x1F); // Keep Alive Clientbound
                    ka.writeLong(id);
                    send_packet(ka);
                }
                // Пакет 0x1A - Player Digging (Копание блоков)
                else if (packetID == 0x1b) {
                    reader.readVarInt(); // Status (0-started, 1-cancelled, 2-finished...)
                    reader.readPosition(); // Block Position (Long / custom read)
                    reader.readByte();   // Face
                }
                // Пакет 0x2E - Player Block Placement (Установка блока)
                else if (packetID == 0x2E) {
                    reader.readVarInt(); // Hand (0: main, 1: offhand)
                    reader.readPosition(); // Block Position
                    reader.readVarInt(); // Face
                    reader.readFloat();  // Cursor X
                    reader.readFloat();  // Cursor Y
                    reader.readFloat();  // Cursor Z
                    reader.readBoolean(); // Inside block
                }
                // Пакет 0x2C - Player Abilities (Клиент шлет подтверждение)
                else if (packetID == 0x1a) {
                    reader.readByte(); // Flags
                }
            }
        }
    } catch (const std::exception& e) { LOG_ERROR("Packet error: " + std::string(e.what())); }
}

void Connection::send_packet(PacketBuffer& packet) {
    int th = Server::get_instance().get_config()["server"]["compression_threshold"].value_or(256);
    send_packet_raw(packet.finalize(compression_enabled, th, crypto_state.get()));
}

void Connection::send_packet_raw(std::vector<uint8_t> data) {
    auto self(shared_from_this());
    auto d = std::make_shared<std::vector<uint8_t>>(std::move(data));
    boost::asio::async_write(socket_, boost::asio::buffer(*d), [self, d](const boost::system::error_code& ec, std::size_t){});
}