#include "network/Connection.h"
#include "console/Logger.h"
#include "game/world/Chunk.h"
#include "game/world/ChunkManager.h"
#include "Server.h"
#include "game/player/PlayerList.h"
#include "game/player/Player.h"
#include "network/PacketRegistry.h"
#include "network/packet/InboundPacket.h"
#include "network/packet/OutboundPacket.h"
#include "network/packet/outbound/play/JoinGamePacket.h"
#include "network/packet/outbound/play/KeepAlivePacketClientbound.h"
#include "network/packet/outbound/play/BrandPacket.h"
#include "network/packet/outbound/play/PlayerInfoPacket.h"
#include "network/packet/outbound/play/SpawnNamedEntityPacket.h"
#include "network/packet/outbound/play/DeclareCommandsPacket.h"
#include "network/packet/outbound/play/EntityMetadataPacket.h"
#include "network/packet/outbound/play/TimeUpdatePacket.h"
#include "network/packet/outbound/play/PlayerPositionAndLookPacket.h"
#include "network/packet/outbound/play/DisconnectPacketPlay.h"
#include "network/packet/outbound/play/EntityEquipmentPacket.h"
#include "network/packet/outbound/play/ChunkDataPacket.h"
#include "network/Metadata.h"
#include "game/nbt/NbtBuilder.h"

#include <toml++/toml.hpp>
#include <nlohmann/json.hpp>
#include <zlib.h>
#include <fstream>
#include <chrono>
#include <cmath>

using json = nlohmann::json;

Connection::Connection(boost::asio::io_context& io_context)
    : socket_(io_context),
      keep_alive_timer_(io_context),
      write_strand_(io_context),
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

void Connection::set_protocol_version(int version) { protocol_version = version; }

int Connection::get_protocol_version() const { return protocol_version; }

void Connection::set_verify_token(const std::vector<uint8_t>& token) {
    this->verify_token = std::make_unique<std::vector<uint8_t>>(token);
}

std::vector<uint8_t> Connection::get_verify_token() const {
    return verify_token ? *verify_token : std::vector<uint8_t>();
}

void Connection::set_waiting_for_encryption_response(bool waiting) {
    this->waiting_for_encryption_response_ = waiting;
}

bool Connection::is_waiting_for_encryption_response() const {
    return waiting_for_encryption_response_;
}

bool Connection::is_connected() const {
    return player != nullptr;
}

void Connection::enable_encryption(const std::vector<uint8_t>& shared_secret) {
    crypto_state = std::make_unique<CryptoState>(aes::init_crypto(shared_secret));
    encrypt = true;
}

boost::asio::io_context::strand& Connection::get_write_strand() {
    return write_strand_;
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
                keep_alive_timer_.cancel();
                if (player) {
                    Server::get_instance().post_game_task([self]() {
                        if (self->getPlayer()) {
                            self->getPlayer()->disconnect();
                        }
                    });
                }
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
            if (state_ == State::PLAY && packetID == 0x10) {
                LOG_DEBUG("Inbound packet 0x10 (KeepAlive) received");
            }
            auto packet = server.get_packet_registry().createPacket(state_, packetID);
            
            if (packet) {
                packet->read(packetReader);
                packet->handle(*this);
            }

            incoming_buffer_.erase(incoming_buffer_.begin(), incoming_buffer_.begin() + varIntLen + packetLength);
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Buffer processing error: " + std::string(e.what()));
        keep_alive_timer_.cancel();
        if (player) {
            Server::get_instance().post_game_task([self = shared_from_this()]() {
                if (self->getPlayer()) {
                    self->getPlayer()->disconnect();
                }
            });
        }
    }
}

void Connection::start_keep_alive_timer() {
    auto self(shared_from_this());
    keep_alive_timer_.expires_after(std::chrono::seconds(30));
    keep_alive_timer_.async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec && state_ == State::PLAY) {
            if (waiting_for_keep_alive_) {
                DisconnectPacketPlay disconnect("Timed out!");
                send_packet(disconnect);
                LOG_WARN("KeepAlive timeout for " + self->get_nickname());
                Server::get_instance().post_game_task([self]() {
                    if (self->getPlayer()) {
                        self->getPlayer()->disconnect();
                    }
                });
                return;
            }
            send_keep_alive();
            start_keep_alive_timer();
        }
    });
}

void Connection::send_keep_alive() {
    last_keep_alive_id_ = std::chrono::system_clock::now().time_since_epoch().count();
    last_keep_alive_sent_ = std::chrono::steady_clock::now();
    waiting_for_keep_alive_ = true;
    KeepAlivePacketClientbound ka(last_keep_alive_id_);
    LOG_DEBUG("Send KeepAlive id=" + std::to_string(last_keep_alive_id_));
    send_packet(ka);
}

void Connection::handle_keep_alive(uint64_t id) {
    if (id == last_keep_alive_id_) {
        waiting_for_keep_alive_ = false;
        auto now = std::chrono::steady_clock::now();
        ping_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_keep_alive_sent_).count();
        LOG_DEBUG("Recv KeepAlive id=" + std::to_string(id) + " ping=" + std::to_string(ping_ms_) + "ms");
    } else {
        LOG_DEBUG("Recv KeepAlive mismatched id=" + std::to_string(id) +
                  " expected=" + std::to_string(last_keep_alive_id_));
    }
}

int Connection::getPing() const {
    return ping_ms_;
}

void Connection::send_join_game() {
    JoinGamePacket joinGamePacket(player, Server::get_instance().get_world());
    send_packet(joinGamePacket);

    BrandPacket brand("Aetherius");
    send_packet(brand);

    DeclareCommandsPacket commandsPacket(Server::get_instance().get_command_registry());
    send_packet(commandsPacket);

    Vector3 pos = player->getPosition();
    Vector2 rot = player->getRotation();
    PlayerPositionAndLookPacket posLookPacket(pos.x, pos.y, pos.z, rot.x, rot.y, 0x00, player->getId());
    send_packet(posLookPacket);

    Metadata ownMetadata;
    ownMetadata.addByte(16, player->getDisplayedSkinParts());
    EntityMetadataPacket ownMeta(player->getId(), ownMetadata);
    send_packet(ownMeta);

    World& world = Server::get_instance().get_world();
    TimeUpdatePacket timePacket(world.getWorldAge(), world.getTimeOfDay());
    send_packet(timePacket);

    Server::get_instance().post_game_task([self = shared_from_this()]() {
        self->broadcast_player_join();
    });
}

void Connection::broadcast_player_join() {
    const auto& allPlayers = PlayerList::getInstance().getPlayers();
    std::vector<std::shared_ptr<Player>> existingPlayers;
    for (const auto& p : allPlayers) {
        if (p->getId() != player->getId()) {
            existingPlayers.push_back(p);
        }
    }

    PlayerInfoPacket addCurrentToAll(PlayerInfoPacket::ADD_PLAYER, {player});
    PlayerInfoPacket addExistingToCurrent(PlayerInfoPacket::ADD_PLAYER, allPlayers);
    send_packet(addExistingToCurrent);

    if (!existingPlayers.empty()) {
        SpawnNamedEntityPacket spawnCurrent(player);
        Metadata currentMetadata;
        currentMetadata.addByte(16, player->getDisplayedSkinParts());
        EntityMetadataPacket currentMeta(player->getId(), currentMetadata);

        // Create equipment packet for the new player
        std::vector<std::pair<EquipmentSlot, Slot>> currentEquipment = player->getFullEquipment();

        for (const auto& p : existingPlayers) {
            p->getConnection()->send_packet(addCurrentToAll);
            p->getConnection()->send_packet(spawnCurrent);
            p->getConnection()->send_packet(currentMeta);
            for(const auto& equip : currentEquipment) {
                EntityEquipmentPacket packet(player->getId(), equip.first, equip.second);
                p->getConnection()->send_packet(packet);
            }

            SpawnNamedEntityPacket spawnExisting(p);
            send_packet(spawnExisting);

            Metadata existingMetadata;
            existingMetadata.addByte(16, p->getDisplayedSkinParts());
            EntityMetadataPacket existingMeta(p->getId(), existingMetadata);
            send_packet(existingMeta);

            // Create and send equipment packet for existing players
            std::vector<std::pair<EquipmentSlot, Slot>> existingEquipment = p->getFullEquipment();
            for(const auto& equip : existingEquipment) {
                EntityEquipmentPacket packet(p->getId(), equip.first, equip.second);
                send_packet(packet);
            }
        }
    }

    LOG_INFO(nickname + " (" + std::to_string(player->getId()) + ") joined the game");
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

    boost::asio::post(write_strand_, [this, self, d]() {
        write_queue_.push(d);
        if (!writing_) {
            do_write();
        }
    });
}

void Connection::do_write() {
    if (write_queue_.empty()) {
        writing_ = false;
        return;
    }

    writing_ = true;
    auto self(shared_from_this());
    auto data = write_queue_.front();

    boost::asio::async_write(socket_, boost::asio::buffer(*data),
        boost::asio::bind_executor(write_strand_,
            [this, self, data](const boost::system::error_code& ec, std::size_t) {
                if (!ec) {
                    write_queue_.pop();
                    do_write();
                } else {
                    writing_ = false;
                    keep_alive_timer_.cancel();
                    if (player) {
                        Server::get_instance().post_game_task([self]() {
                            if (self->getPlayer()) {
                                self->getPlayer()->disconnect();
                            }
                        });
                    }
                }
            }));
}

void Connection::send_light_data(int chunkX, int chunkZ) {}

void Connection::update_chunks() {
    if (!player) return;

    Server& server = Server::get_instance();
    World& world = server.get_world();

    // CRITICAL: Process completed chunks FIRST to make them available
    world.flushCompletedChunks();

    Vector3 pos = player->getPosition();
    int chunkX = static_cast<int>(std::floor(pos.x / 16.0));
    int chunkZ = static_cast<int>(std::floor(pos.z / 16.0));

    if (chunkX == last_chunk_x_ && chunkZ == last_chunk_z_) {
        // Still process completed chunks even if not moving
        return;
    }

    PacketBuffer vp;
    vp.writeVarInt(0x40); // Update View Position
    vp.writeVarInt(chunkX);
    vp.writeVarInt(chunkZ);
    send_packet(vp);

    int viewDistance = player->getViewDistance();
    const int MAX_VIEW_DISTANCE = 10;
    if (viewDistance > MAX_VIEW_DISTANCE) {
        viewDistance = MAX_VIEW_DISTANCE;
    }

    // Clean up chunks that left view distance
    std::vector<std::pair<int, int>> to_remove;
    {
        std::lock_guard<std::mutex> lock(sent_chunks_mutex_);
        for (const auto& [cx, cz] : sent_chunks_) {
            if (std::abs(cx - chunkX) > viewDistance || std::abs(cz - chunkZ) > viewDistance) {
                to_remove.emplace_back(cx, cz);
            }
        }
        for (const auto& coord : to_remove) {
            sent_chunks_.erase(coord);
        }
    }

    // Build list of chunks to load with spiral priority
    std::vector<std::tuple<int, int, int>> chunks_to_load;

    {
        std::lock_guard<std::mutex> lock(sent_chunks_mutex_);
        for (int x = -viewDistance; x <= viewDistance; ++x) {
            for (int z = -viewDistance; z <= viewDistance; ++z) {
                int cx = chunkX + x;
                int cz = chunkZ + z;

                bool alreadySent = sent_chunks_.contains({cx, cz});

                if (!alreadySent) {
                    int priority = ChunkManager::calculatePriority(cx, cz, chunkX, chunkZ);
                    chunks_to_load.emplace_back(cx, cz, priority);
                }
            }
        }
    }

    // Sort by priority (closest chunks first)
    std::sort(chunks_to_load.begin(), chunks_to_load.end(),
        [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });

    // Throttle chunk sending to avoid client timeouts
    const size_t BATCH_SIZE = 32;
    const size_t MAX_NEW_CHUNK_REQUESTS = 128;

    int sent_immediately = 0;
    int requested = 0;

    // Pass 1: Send ALL already-loaded chunks in large batches (ULTRA FAST)
    std::vector<std::vector<uint8_t>> batch_raw;
    batch_raw.reserve(BATCH_SIZE);

    // Static data shared across all chunks
    static const std::vector<uint8_t> heightmap_nbt = []() {
        NbtBuilder heightmapNbt;
        heightmapNbt.startCompound();
        heightmapNbt.startCompound("MOTION_BLOCKING");
        std::vector<int64_t> motion_blocking(37, 0);
        heightmapNbt.writeLongArray("MOTION_BLOCKING", motion_blocking);
        heightmapNbt.endCompound();
        heightmapNbt.endCompound();
        return heightmapNbt.buffer;
    }();

    for (const auto& [cx, cz, priority] : chunks_to_load) {
        if (auto* chunk = world.getChunk(cx, cz)) {
            // Ultra-fast serialization using pre-cached data
            PacketBuffer buffer;
            buffer.writeVarInt(0x20);
            buffer.writeInt(cx);
            buffer.writeInt(cz);
            buffer.writeBoolean(true);

            int primaryBitmask = 0;
            for (int i = 0; i < 16; ++i) {
                if (chunk->getSection(i) != nullptr && !chunk->getSection(i)->isEmpty()) {
                    primaryBitmask |= (1 << i);
                }
            }
            buffer.writeVarInt(primaryBitmask);

            buffer.writeNbt(heightmap_nbt);

            // Optimized biome writing
            buffer.writeVarInt(1024);
            for (int i = 0; i < 1024; ++i) {
                buffer.writeVarInt(1);
            }

            // Use pre-cached chunk section data
            auto cachedData = chunk->getCachedNetworkData();
            buffer.writeVarInt(cachedData.size());
            buffer.data.insert(buffer.data.end(), cachedData.begin(), cachedData.end());
            buffer.writeVarInt(0);

            // Finalize immediately
            int th = Server::get_instance().get_config()["server"]["compression_threshold"].value_or(256);
            batch_raw.push_back(buffer.finalize(compression_enabled, th, crypto_state.get()));

            {
                std::lock_guard<std::mutex> lock(sent_chunks_mutex_);
                sent_chunks_.insert({cx, cz});
            }

            sent_immediately++;

            // Send large batch when full
            if (batch_raw.size() >= BATCH_SIZE) {
                auto batch_copy = std::make_shared<std::vector<std::vector<uint8_t>>>(std::move(batch_raw));
                batch_raw.clear();
                batch_raw.reserve(BATCH_SIZE);

                boost::asio::post(write_strand_, [this, self = shared_from_this(), batch_copy]() {
                    if (socket_.is_open()) {
                        for (const auto& data : *batch_copy) {
                            write_queue_.push(std::make_shared<std::vector<uint8_t>>(data));
                        }
                        if (!writing_) {
                            do_write();
                        }
                    }
                });
            }
        }
    }

    // Send remaining batch
    if (!batch_raw.empty()) {
        auto batch_copy = std::make_shared<std::vector<std::vector<uint8_t>>>(std::move(batch_raw));
        boost::asio::post(write_strand_, [this, self = shared_from_this(), batch_copy]() {
            if (socket_.is_open()) {
                for (const auto& data : *batch_copy) {
                    write_queue_.push(std::make_shared<std::vector<uint8_t>>(data));
                }
                if (!writing_) {
                    do_write();
                }
            }
        });
    }

    // Pass 2: Request massive number of chunks in parallel (with deduplication)
    for (const auto& [cx, cz, priority] : chunks_to_load) {
        if (requested >= MAX_NEW_CHUNK_REQUESTS) break;

        if (!world.getChunk(cx, cz)) {
            // Check if we already requested this chunk
            bool already_pending = false;
            {
                std::lock_guard<std::mutex> lock(pending_requests_mutex_);
                if (pending_chunk_requests_.contains({cx, cz})) {
                    already_pending = true;
                } else {
                    pending_chunk_requests_.insert({cx, cz});
                }
            }

            if (already_pending) {
                continue; // Skip duplicate request
            }

            LOG_DEBUG("Chunk " + std::to_string(cx) + "," + std::to_string(cz) +
                      " not loaded, requesting generation/load");

            world.requestChunk(cx, cz, priority, [this, self = shared_from_this(), cx, cz](ChunkColumn* chunk) {
                // This callback runs in processCompletedChunks() - serialize immediately
                // Remove from pending list
                {
                    std::lock_guard<std::mutex> lock(pending_requests_mutex_);
                    pending_chunk_requests_.erase({cx, cz});
                }

                if (!socket_.is_open() || !chunk) return;

                // Mark as sent IMMEDIATELY to prevent re-requests
                {
                    std::lock_guard<std::mutex> lock(sent_chunks_mutex_);
                    sent_chunks_.insert({cx, cz});
                }

                LOG_DEBUG("Chunk " + std::to_string(cx) + "," + std::to_string(cz) +
                          " ready, sending to client");

                // Serialize immediately (network cache is already pre-computed)
                PacketBuffer buffer;
                buffer.writeVarInt(0x20);
                buffer.writeInt(cx);
                buffer.writeInt(cz);
                buffer.writeBoolean(true);

                int primaryBitmask = 0;
                for (int i = 0; i < 16; ++i) {
                    if (chunk->getSection(i) != nullptr && !chunk->getSection(i)->isEmpty()) {
                        primaryBitmask |= (1 << i);
                    }
                }
                buffer.writeVarInt(primaryBitmask);

                static const std::vector<uint8_t> heightmap_nbt = []() {
                    NbtBuilder heightmapNbt;
                    heightmapNbt.startCompound();
                    heightmapNbt.startCompound("MOTION_BLOCKING");
                    std::vector<int64_t> motion_blocking(37, 0);
                    heightmapNbt.writeLongArray("MOTION_BLOCKING", motion_blocking);
                    heightmapNbt.endCompound();
                    heightmapNbt.endCompound();
                    return heightmapNbt.buffer;
                }();
                buffer.writeNbt(heightmap_nbt);

                buffer.writeVarInt(1024);
                for (int i = 0; i < 1024; ++i) {
                    buffer.writeVarInt(1);
                }

                // Pre-cached data is already computed in worker thread
                auto cachedData = chunk->getCachedNetworkData();
                buffer.writeVarInt(cachedData.size());
                buffer.data.insert(buffer.data.end(), cachedData.begin(), cachedData.end());
                buffer.writeVarInt(0);

                // Finalize and queue for async write
                int th = Server::get_instance().get_config()["server"]["compression_threshold"].value_or(256);
                auto finalized = std::make_shared<std::vector<uint8_t>>(
                    buffer.finalize(compression_enabled, th, crypto_state.get())
                );

                // Only the actual write goes to the strand
                boost::asio::post(write_strand_, [this, self, finalized]() {
                    if (socket_.is_open()) {
                        write_queue_.push(finalized);
                        if (!writing_) {
                            do_write();
                        }
                    }
                });
            });
            requested++;
        }
    }

    int total_needed = chunks_to_load.size();
    int deferred = total_needed - sent_immediately - requested;

    if (sent_immediately > 0 || requested > 0) {
        ChunkManager& cm = world.getChunkManager();
        LOG_DEBUG("Chunks: " + std::to_string(sent_immediately) + " sent, " +
                  std::to_string(requested) + " requested (" +
                  std::to_string(deferred) + " deferred) | loaded=" +
                  std::to_string(cm.getLoadedCount()) + " pending=" +
                  std::to_string(cm.getPendingCount()) + " queued=" +
                  std::to_string(cm.getQueuedCount()) + " completed=" +
                  std::to_string(cm.getCompletedCount()));
    }

    last_chunk_x_ = chunkX;
    last_chunk_z_ = chunkZ;
}
