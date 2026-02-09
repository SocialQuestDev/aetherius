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

void Connection::start() { do_read(); }

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
            } else {
                if (player) {
                    PlayerList::getInstance().removePlayer(player->getId());
                }
                LOG_DEBUG("Connection closed: " + ec.message());
            }
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
    KeepAlivePacketClientbound ka(last_keep_alive_id_);
    send_packet(ka);
}

void Connection::send_join_game() {
    JoinGamePacket joinGamePacket(player->getId(), Server::get_instance().get_world());
    send_packet(joinGamePacket);

    BrandPacket brand("Aetherius");
    send_packet(brand);

    player->teleportToSpawn();
}

void Connection::handle_packet(std::vector<uint8_t>& rawData) {
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
                LOG_WARN("Unhandled packet ID: 0x" + std::to_string(packetID) + " in state " + std::to_string((int)state_));
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Packet error: " + std::string(e.what()));
    }
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