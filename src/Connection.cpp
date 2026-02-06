#include "../include/Connection.h"
#include "../include/PacketBuffer.h"
#include "../include/Logger.h"
#include "../include/Chunk.h"
#include "../include/World.h"
#include "../include/Server.h"

#include <toml++/toml.hpp>

Connection::Connection(boost::asio::io_context& io_context)
    : socket_(io_context) {}

std::shared_ptr<Connection> Connection::create(boost::asio::io_context& io_context) {
    return std::shared_ptr<Connection>(new Connection(io_context));
}

tcp::socket& Connection::socket() {
    return socket_;
}

void Connection::start() {
    do_read();
}

static std::string base64_encode_local(const std::vector<unsigned char>& data) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
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
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
    return "data:image/png;base64," + base64_encode_local(buffer);
}

void Connection::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(buffer_),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::vector<uint8_t> received(buffer_, buffer_ + length);

                try {
                    handle_packet(received);
                } catch (std::exception& e) {
                    LOG_ERROR("Packet parsing error: " + std::string(e.what()));
                }

                do_read();
            }
        });
}

void Connection::handle_packet(std::vector<uint8_t>& rawData) {
    PacketBuffer reader(rawData);

    try {
        while (reader.readerIndex < reader.data.size()) {

            size_t startPos = reader.readerIndex;
            int length = reader.readVarInt();
            int packetID = reader.readVarInt();

            LOG_DEBUG("State: " + std::to_string((int)state_) + " | Packet ID: " + std::to_string(packetID));

            if (state_ == State::HANDSHAKE) {
                if (packetID == 0x00) { // Handshake Packet
                    int protocolVersion = reader.readVarInt();
                    std::string serverAddr = reader.readString();
                    unsigned short serverPort = reader.readUShort();
                    int nextState = reader.readVarInt();

                    LOG_DEBUG("Handshake: Protocol " + std::to_string(protocolVersion) +
                                 " NextState: " + std::to_string(nextState));

                    if (nextState == 1) state_ = State::STATUS;
                    else if (nextState == 2) state_ = State::LOGIN;
                }
            }
            if (state_ == State::STATUS) {
                if (packetID == 0x00) {
                    auto config = toml::parse_file("config.toml");

                    std::string motd = config["server"]["motd"].value_or("Aetherius Server");
                    int maxPlayers = config["server"]["max_players"].value_or(20);
                    std::string icon = getIconBase64(config["server"]["icon_path"].value_or("server-icon.png"));

                    std::string jsonResponse = "{";
                    jsonResponse += "\"version\": {\"name\": \"1.16.5\", \"protocol\": 754},";
                    jsonResponse += "\"players\": {\"max\": " + std::to_string(maxPlayers) + ", \"online\": 0, \"sample\": []},";
                    jsonResponse += "\"description\": {\"text\": \"" + motd + "\"}";
                    if (!icon.empty()) {
                        jsonResponse += ", \"favicon\": \"" + icon + "\"";
                    }
                    jsonResponse += "}";

                    PacketBuffer res;
                    res.writeVarInt(0x00);
                    res.writeString(jsonResponse);
                    send_packet(res.finalize());
                }
                else if (packetID == 0x01) {
                    int64_t payload = reader.readLong();
                    PacketBuffer pong;
                    pong.writeVarInt(0x01);
                    pong.writeLong(payload);
                    send_packet(pong.finalize());
                }
            }
            else if (state_ == State::LOGIN) {
                if (packetID == 0x00) {
                    std::string playerName = reader.readString();
                    LOG_INFO("Player logging in: " + playerName);

                    PacketBuffer loginSuccess;
                    loginSuccess.writeVarInt(0x02);
                    loginSuccess.writeUUID(0, 0);
                    loginSuccess.writeString(playerName);
                    send_packet(loginSuccess.finalize());

                    LOG_DEBUG("Sent Login Success");

                    state_ = State::PLAY;

                    PacketBuffer disconnect;
                    disconnect.writeVarInt(0x19);
                    disconnect.writeString("{\"text\":\"Aetherius: Ты почти зашел!\"}");
                    send_packet(disconnect.finalize());

                    /*
                    PacketBuffer joinGame;
                    joinGame.writeVarInt(0x25);
                    joinGame.writeInt(123);
                    joinGame.data.push_back(0);
                    joinGame.data.push_back(1);
                    joinGame.data.push_back(-1);
                    joinGame.writeVarInt(1);
                    joinGame.writeString("minecraft:overworld");

                    auto codec = getDimensionCodec();
                    joinGame.data.insert(joinGame.data.end(), codec.begin(), codec.end());

                    NbtBuilder dimNbt;
                    dimNbt.writeByte(TAG_COMPOUND); dimNbt.writeString("");
                    dimNbt.writeTagString("name", "minecraft:overworld");
                    dimNbt.writeTagByte("piglin_safe", 0);
                    dimNbt.writeTagByte("natural", 1);
                    dimNbt.writeTagFloat("ambient_light", 0.0f);
                    dimNbt.writeTagString("infiniburn", "minecraft:infiniburn_overworld");
                    dimNbt.writeTagString("effects", "minecraft:overworld");
                    dimNbt.writeTagByte("has_raids", 1);
                    dimNbt.writeTagByte("respawn_anchor_works", 0);
                    dimNbt.writeTagByte("bed_works", 1);
                    dimNbt.writeTagByte("has_skylight", 1);
                    dimNbt.writeTagByte("has_ceiling", 0);
                    dimNbt.writeTagByte("ultrawarm", 0);
                    dimNbt.writeTagInt("logical_height", 256);
                    dimNbt.writeTagInt("coordinate_scale", 1);
                    dimNbt.endCompound();
                    joinGame.data.insert(joinGame.data.end(), dimNbt.buffer.begin(), dimNbt.buffer.end());

                    joinGame.writeString("minecraft:overworld");
                    joinGame.writeLong(12345678);
                    joinGame.writeVarInt(20);
                    joinGame.writeVarInt(10);
                    joinGame.data.push_back(0);
                    joinGame.data.push_back(1);
                    joinGame.data.push_back(0);
                    joinGame.data.push_back(1);

                    send_packet(joinGame.finalize());

                    PacketBuffer pos;
                    pos.writeVarInt(0x34);
                    double x=0, y=100, z=0;
                    auto writeDouble = [&](double d) {
                        uint64_t val;
                        std::memcpy(&val, &d, 8);
                        for(int i=7; i>=0; i--) pos.data.push_back((val >> (i*8)) & 0xFF);
                    };
                    writeDouble(x); writeDouble(y); writeDouble(z);
                    float yaw=0, pitch=0;
                    pos.data.push_back(0);
                    pos.writeVarInt(0);
                    send_packet(pos.finalize());

                    ChunkColumn chunk;
                    chunk.x = 0; chunk.z = 0;
                    std::vector<uint8_t> chunkData = chunk.serialize();

                    PacketBuffer chunkPacket;
                    chunkPacket.writeVarInt(0x20);
                    chunkPacket.data.insert(chunkPacket.data.end(), chunkData.begin(), chunkData.end());
                    send_packet(chunkPacket.finalize());*/
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Packet error: " + std::string(e.what()));
    }
}

void Connection::send_packet(std::vector<uint8_t> packetData) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(packetData),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (ec) LOG_ERROR("Send failed");
        });
}