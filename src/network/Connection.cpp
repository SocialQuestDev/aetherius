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
#include <chrono>

using json = nlohmann::json;

Connection::Connection(boost::asio::io_context& io_context) 
    : socket_(io_context), 
      keep_alive_timer_(io_context),
      buffer_{},
      is_dead(false) {}

std::shared_ptr<Connection> Connection::create(boost::asio::io_context& io_context) {
    return std::shared_ptr<Connection>(new Connection(io_context));
}

tcp::socket& Connection::socket() { return socket_; }

void Connection::start() { do_read(); }

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

void Connection::start_keep_alive_timer() {
    auto self(shared_from_this());
    keep_alive_timer_.expires_after(std::chrono::seconds(15));
    keep_alive_timer_.async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec && state_ == State::PLAY) {
            send_keep_alive();
            start_keep_alive_timer();
        }
    });
}

void Connection::send_keep_alive() {
    last_keep_alive_id_ = std::chrono::system_clock::now().time_since_epoch().count();
    
    PacketBuffer ka;
    ka.writeVarInt(0x1f); // packet_keep_alive (Clientbound)
    ka.writeLong(last_keep_alive_id_);
    
    send_packet(ka);
}

void Connection::send_join_game() {
    PacketBuffer joinGame;
    joinGame.writeVarInt(0x24); // packet_login
    joinGame.writeInt(1); 
    joinGame.writeBoolean(false); 
    joinGame.writeByte(1); // Gamemode: Creative
    joinGame.writeByte(255); // Previous Gamemode
    joinGame.writeVarInt(1); 
    joinGame.writeString("minecraft:overworld");
    
    World& world = Server::get_instance().get_world();
    joinGame.writeNbt(world.getDimensionCodec());
    joinGame.writeNbt(world.getDimension());
    
    joinGame.writeString("minecraft:overworld");
    joinGame.writeLong(0); // Hashed seed
    joinGame.writeVarInt(20); 
    joinGame.writeVarInt(10); 
    joinGame.writeBoolean(false); 
    joinGame.writeBoolean(true); 
    joinGame.writeBoolean(false); 
    joinGame.writeBoolean(false); 
    send_packet(joinGame);

    teleport_to_spawn();
}

void Connection::teleport_to_spawn() {
    is_dead = false;

    // packet_update_health: 0x4A
    PacketBuffer health;
    health.writeVarInt(0x49); 
    health.writeFloat(20.0f);
    health.writeVarInt(20);
    health.writeFloat(5.0f);
    send_packet(health);

    // packet_position: 0x35
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
    LOG_DEBUG("Player teleported to spawn and healed");
}

void Connection::kill_player() {
    if (is_dead) return;
    is_dead = true;

    LOG_DEBUG("Player fell into the void. Sending death status and zero health.");
    
    // packet_update_health: 0x4A
    PacketBuffer health;
    health.writeVarInt(0x49); 
    health.writeFloat(0.0f);  
    health.writeVarInt(0);    
    health.writeFloat(0.0f);  
    send_packet(health);

    // packet_entity_status: 0x1B
    PacketBuffer death;
    death.writeVarInt(0x1A); 
    death.writeInt(1);       
    death.writeByte(3);      
    send_packet(death);

    // packet_chat: 0x3B
    PacketBuffer chat;
    chat.writeVarInt(0x0E); 
    chat.writeString("{\"text\":\"Вы упали в бездну!\", \"color\":\"red\"}");
    chat.writeByte(1);      
    chat.writeUUID(get_offline_UUID_128(nickname));
    send_packet(chat);
}

void Connection::handle_packet(std::vector<uint8_t>& rawData) {
    PacketBuffer raw(rawData);
    try {
        Server& server = Server::get_instance();
        const toml::table& serverCfg = server.get_config();

        while (raw.readerIndex < raw.data.size()) {
            int packetLength = raw.readVarInt();
            if (packetLength < 1 || (raw.readerIndex + packetLength) > raw.data.size()) break;

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

            if (state_ == State::HANDSHAKE) {
                if (packetID == 0x00) {
                    reader.readVarInt(); reader.readString(); reader.readUShort();
                    int next = reader.readVarInt();
                    state_ = (next == 1) ? State::STATUS : State::LOGIN;
                }
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
            else if (state_ == State::LOGIN) {
                if (packetID == 0x00) {
                    nickname = reader.readString();
                    if (serverCfg["server"]["compression_enabled"].value_or(false)) {
                        int th = serverCfg["server"]["compression_threshold"].value_or(256);
                        PacketBuffer comp; comp.writeVarInt(0x03); comp.writeVarInt(th);
                        send_packet_raw(comp.finalize(false, -1, nullptr));
                        compression_enabled = true;
                    }
                    PacketBuffer succ; succ.writeVarInt(0x02); 
                    succ.writeUUID(get_offline_UUID_128(nickname));
                    succ.writeString(nickname); 
                    send_packet(succ);
                    
                    state_ = State::PLAY; 
                    send_join_game();
                    start_keep_alive_timer();
                }
            } 
            else if (state_ == State::PLAY) {
                if (packetID == 0x10) {
                    reader.readLong();
                }
                else if (packetID == 0x00) { // Teleport Confirm
                    // packet_update_view_position: 0x41
                    PacketBuffer vp; 
                    vp.writeVarInt(0x40); 
                    vp.writeVarInt(0); 
                    vp.writeVarInt(0); 
                    send_packet(vp);

                    World& world = server.get_world();
                    for (int x = -2; x <= 2; ++x) {
                        for (int z = -2; z <= 2; ++z) {
                            ChunkColumn* chunk = world.generateChunk(x, z);
                            PacketBuffer cp; 
                            // packet_map_chunk: 0x21
                            cp.writeVarInt(0x20); 
                            auto p = chunk->serialize();
                            cp.data.insert(cp.data.end(), p.begin(), p.end());
                            send_packet(cp);
                        }
                    }
                }
                else if (packetID == 0x04) { // Client Status
                    int action = reader.readVarInt();
                    if (action == 0) { 
                        LOG_DEBUG("Player requested respawn.");
                        
                        // packet_respawn: 0x3A
                        PacketBuffer rb;
                        rb.writeVarInt(0x39); 
                        rb.writeNbt(server.get_world().getDimension()); 
                        rb.writeString("minecraft:overworld"); 
                        rb.writeLong(0); 
                        rb.writeByte(1); 
                        rb.writeByte(255); 
                        rb.writeBoolean(false); 
                        rb.writeBoolean(false); 
                        rb.writeBoolean(true); 
                        send_packet(rb);

                        teleport_to_spawn();
                    }
                }
                else if (packetID >= 0x12 && packetID <= 0x15) {
                    double x, y, z;
                    bool onGround;
                    
                    if (packetID == 0x12) { 
                        x = reader.readDouble(); y = reader.readDouble(); z = reader.readDouble();
                        onGround = reader.readBoolean();
                    }
                    else if (packetID == 0x13) { 
                        x = reader.readDouble(); y = reader.readDouble(); z = reader.readDouble();
                        reader.readFloat(); reader.readFloat(); 
                        onGround = reader.readBoolean();
                    }
                    else if (packetID == 0x14) { 
                        reader.readFloat(); reader.readFloat(); 
                        onGround = reader.readBoolean();
                        y = 0;
                    }
                    else if (packetID == 0x15) { 
                        onGround = reader.readBoolean();
                        y = 0;
                    }

                    if (!is_dead && (packetID == 0x12 || packetID == 0x13) && y < -10.0) {
                        LOG_DEBUG("Player fell below Y=-10, killing...");
                        kill_player();
                    }
                }
            }
        }
    } catch (const std::exception& e) { 
        LOG_ERROR("Packet error: " + std::string(e.what())); 
    }
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

void Connection::send_light_data(int chunkX, int chunkZ) {}