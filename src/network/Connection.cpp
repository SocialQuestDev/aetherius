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
#include "../../include/network/PacketRegistry.h"
#include "../../include/network/packet/InboundPacket.h"
#include "../../include/network/packet/OutboundPacket.h"
#include "../../include/network/packet/play/JoinGamePacket.h"
#include "../../include/network/packet/play/KeepAlivePacketClientbound.h"
#include "../../include/network/packet/play/BrandPacket.h"
#include "../../include/network/packet/play/PlayerInfoPacket.h"
#include "../../include/network/packet/play/SpawnNamedEntityPacket.h"
#include "../../include/network/packet/play/EntityMetadataPacket.h"
#include "../../include/network/packet/play/PlayerPositionAndLookPacket.h" // Для телепорта

#include <toml++/toml.hpp>
#include <nlohmann/json.hpp>
#include <zlib.h>
#include <fstream>
#include <chrono>

using json = nlohmann::json;

Connection::Connection(boost::asio::io_context& io_context)
    : socket_(io_context),
      keep_alive_timer_(io_context),
      buffer_{} {}

std::shared_ptr<Connection> Connection::create(boost::asio::io_context& io_context) {
    return std::shared_ptr<Connection>(new Connection(io_context));
}

tcp::socket& Connection::socket() { return socket_; }

void Connection::start() { 
    boost::system::error_code ec;
    socket_.set_option(boost::asio::ip::tcp::no_delay(true), ec);
    do_read(); 
}

void Connection::setState(State state) {
    state_ = state;
}

State Connection::getState() const {
    return state_;
}

void Connection::set_compression(bool enabled) {
    compression_enabled = enabled;
}

void Connection::setPlayer(std::shared_ptr<Player> player) {
    this->player = player;
}

std::shared_ptr<Player> Connection::getPlayer() const {
    return player;
}

void Connection::set_nickname(const std::string& name) {
    this->nickname = name;
}

std::string Connection::get_nickname() const {
    return nickname;
}

void Connection::set_verify_token(const std::vector<uint8_t>& token) {
    this->verify_token = std::make_unique<std::vector<uint8_t>>(token);
}

std::vector<uint8_t> Connection::get_verify_token() const {
    return verify_token ? *verify_token : std::vector<uint8_t>();
}

void Connection::set_waiting_for_encryption(bool waiting) {
    this->waiting_for_response = waiting;
}

bool Connection::is_waiting_for_encryption() const {
    return waiting_for_response;
}

bool Connection::is_connected() const {
    return player != nullptr;
}

void Connection::enable_encryption(const std::vector<uint8_t>& shared_secret) {
    crypto_state = std::make_unique<CryptoState>(aes::init_crypto(shared_secret));
    encrypt = true;
}

void Connection::do_read() {
    auto self(shared_from_this());
    
    socket_.async_read_some(boost::asio::buffer(buffer_),
        [this, self](const boost::system::error_code &ec, const std::size_t length) {
            if (!ec) {
                if (encrypt && crypto_state) {
                    aes::decrypt(*crypto_state, buffer_, length);
                }
                incoming_buffer_.insert(incoming_buffer_.end(), buffer_, buffer_ + length);
                process_incoming_buffer();
                do_read();
            } else {
                if (player) PlayerList::getInstance().removePlayer(player->getId());
                LOG_DEBUG("Connection closed: " + ec.message());
            }
        });
}

void Connection::process_incoming_buffer() {
    try {
        Server& server = Server::get_instance();
        
        while (true) {
            if (incoming_buffer_.empty()) break;

            PacketBuffer raw(incoming_buffer_);
            
            int varIntLen = 0;
            int packetLength = 0;
            bool incompleteVarInt = true;

            size_t i = 0;
            int numRead = 0;
            int result = 0;

            for(i = 0; i < incoming_buffer_.size() && i < 5; i++) {
                uint8_t read = incoming_buffer_[i];
                int value = (read & 0b01111111);
                result |= (value << (7 * numRead));
                numRead++;
                if ((read & 0b10000000) == 0) {
                    packetLength = result;
                    varIntLen = numRead;
                    incompleteVarInt = false;
                    break;
                }
            }

            if (incompleteVarInt) return;

            if (incoming_buffer_.size() < (varIntLen + packetLength)) return;

            std::vector<uint8_t> packetData;
            auto dataStart = incoming_buffer_.begin() + varIntLen;
            auto dataEnd = dataStart + packetLength;
            packetData.assign(dataStart, dataEnd);

            PacketBuffer reader(packetData);
            
            std::vector<uint8_t> payload;
            if (compression_enabled) {
                int dataLength = reader.readVarInt();
                if (dataLength > 0) {
                    payload.resize(dataLength);
                    uLongf destLen = dataLength;
                    auto compressedDataStart = packetData.data() + reader.readerIndex;
                    int compressedSize = packetData.size() - reader.readerIndex;
                    uncompress(payload.data(), &destLen, compressedDataStart, compressedSize);
                } else {
                     payload.assign(packetData.begin() + reader.readerIndex, packetData.end());
                }
            } else {
                payload = packetData;
            }

            PacketBuffer packetReader(payload);
            int packetID = packetReader.readVarInt();
            auto packet = server.get_packet_registry().createPacket(state_, packetID);
            
            if (packet) {
                packet->read(packetReader);
                packet->handle(*this);
            }

            incoming_buffer_.erase(incoming_buffer_.begin(), incoming_buffer_.begin() + varIntLen + packetLength);
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Buffer processing error: " + std::string(e.what()));
        socket_.close();
        if (player) PlayerList::getInstance().removePlayer(player->getId());
    }
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
    KeepAlivePacketClientbound ka(last_keep_alive_id_);
    send_packet(ka);
}

void Connection::send_join_game() {
    // 1. Отправляем JoinGame и Brand самому игроку
    JoinGamePacket joinGamePacket(player->getId(), Server::get_instance().get_world());
    send_packet(joinGamePacket);

    BrandPacket brand("Aetherius");
    send_packet(brand);

    player->teleportToSpawn();

    if (!PlayerList::getInstance().getPlayers().empty()) {
        PlayerInfoPacket newPlayerPacket(PlayerInfoPacket::ADD_PLAYER, {player}); //govnokod by like
        for (const auto& p : PlayerList::getInstance().getPlayers()) {
            p->getConnection()->send_packet(newPlayerPacket);
        }

        SpawnNamedEntityPacket spawnPlayerPacket(player); //govnokod by like
        for (const auto& p : PlayerList::getInstance().getPlayers()) {
            if (p->getId() != player->getId()) {
                p->getConnection()->send_packet(spawnPlayerPacket);
            }
        }

        //for (const auto& p : PlayerList::getInstance().getPlayers()) {
        //    if (p->getId() != player->getId()) {
        //        SpawnNamedEntityPacket spawnAllPlayersPacket(player);
        //        p->getConnection()->send_packet(spawnAllPlayersPacket);
        //    }
        //}

        PlayerInfoPacket allPlayersPacket(PlayerInfoPacket::ADD_PLAYER, PlayerList::getInstance().getPlayers());
        send_packet(allPlayersPacket);
    }

    LOG_INFO(nickname + " (" + std::to_string(player->getId()) + ") joined the game");
}


void Connection::handle_packet(std::vector<uint8_t>& rawData) {
    // Этот метод, похоже, дублирует process_incoming_buffer. Оставляю его как есть.
    PacketBuffer raw(rawData);
    try {
        Server& server = Server::get_instance();

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

            auto packet = server.get_packet_registry().createPacket(state_, packetID);
            if (packet) {
                packet->read(reader);
                packet->handle(*this);
            } else {
                LOG_WARN("Unhandled packet ID: " + std::to_string(packetID) + " in state " + std::to_string((int)state_));
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Packet error: " + std::string(e.what()));
    }
}

std::vector<uint8_t> Connection::finalize_packet(PacketBuffer& packet){
    int th = Server::get_instance().get_config()["server"]["compression_threshold"].value_or(256);
    return packet.finalize(compression_enabled, th, crypto_state.get());
}

void Connection::send_packet(OutboundPacket& packet) {
    PacketBuffer buffer;
    buffer.writeVarInt(packet.getPacketId());
    packet.write(buffer);
    send_packet(buffer);
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