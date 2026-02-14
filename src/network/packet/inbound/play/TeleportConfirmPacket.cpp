#include "network/packet/inbound/play/TeleportConfirmPacket.h"
#include "network/Connection.h"
#include "Server.h"
#include "game/world/World.h"
#include "game/world/Chunk.h"
#include "game/world/ChunkManager.h"
#include "game/player/Player.h"
#include "network/packet/outbound/play/ChunkDataPacket.h"
#include <cmath>
#include <boost/asio.hpp>
#include <functional>
#include <vector>
#include <memory>
#include <algorithm>
#include <atomic>
#include <map>

class RingedChunkSender : public std::enable_shared_from_this<RingedChunkSender> {
public:
    RingedChunkSender(std::shared_ptr<Connection> conn, int centerChunkX, int centerChunkZ)
        : conn_(conn),
          io_context_(Server::get_instance().get_io_context()),
          world_(Server::get_instance().get_world()),
          timer_(io_context_) {

        int viewDistance = conn->getPlayer()->getViewDistance();
        if (viewDistance > 10) viewDistance = 10;

        std::map<int, std::vector<std::pair<int, int>>> rings;
        for (int z = -viewDistance; z <= viewDistance; ++z) {
            for (int x = -viewDistance; x <= viewDistance; ++x) {
                int dist = std::max(std::abs(x), std::abs(z));
                rings[dist].push_back({centerChunkX + x, centerChunkZ + z});
            }
        }

        for (int i = 0; i <= viewDistance; ++i) {
            if (rings.count(i)) {
                rings_to_load_.push_back(std::move(rings[i]));
            }
        }
    }

    void start() {
        schedule_next_ring(std::chrono::milliseconds(0));
    }

private:
    void schedule_next_ring(std::chrono::milliseconds delay) {
        timer_.expires_after(delay);
        timer_.async_wait([self = shared_from_this()](const boost::system::error_code& ec) {
            if (!ec) {
                self->process_ring();
            }
        });
    }

    void process_ring() {
        if (rings_to_load_.empty() || !conn_->is_connected()) {
            return;
        }

        auto ring = rings_to_load_.front();
        rings_to_load_.erase(rings_to_load_.begin());

        auto completion_counter = std::make_shared<std::atomic<size_t>>(0);
        size_t ring_size = ring.size();

        for (const auto& coord : ring) {
            int priority = ChunkManager::calculatePriority(coord.first, coord.second,
                                                          conn_->getPlayer()->getPosition().x / 16.0,
                                                          conn_->getPlayer()->getPosition().z / 16.0);

            world_.requestChunk(coord.first, coord.second, priority, [self = shared_from_this(), completion_counter, ring_size](ChunkColumn* chunk) {
                if (chunk) {
                    auto packet = std::make_shared<ChunkDataPacket>(chunk);

                    boost::asio::post(self->conn_->get_write_strand(), [self, packet]() {
                        if (!self->conn_->is_connected()) return;
                        self->conn_->send_packet(*packet);
                    });
                }

                if (completion_counter->fetch_add(1) + 1 == ring_size) {
                    self->schedule_next_ring(std::chrono::milliseconds(30));
                }
            });
        }
    }

    std::shared_ptr<Connection> conn_;
    boost::asio::io_context& io_context_;
    World& world_;
    std::vector<std::vector<std::pair<int, int>>> rings_to_load_;
    boost::asio::steady_timer timer_;
};

void TeleportConfirmPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (!player) return;

    Vector3 pos = player->getPosition();
    int chunkX = static_cast<int>(std::floor(pos.x / 16.0));
    int chunkZ = static_cast<int>(std::floor(pos.z / 16.0));

    PacketBuffer vp;
    vp.writeVarInt(0x40);
    vp.writeVarInt(chunkX);
    vp.writeVarInt(chunkZ);
    connection.send_packet(vp);

    std::make_shared<RingedChunkSender>(connection.shared_from_this(), chunkX, chunkZ)->start();
}

void TeleportConfirmPacket::read(PacketBuffer& buffer) {
    teleportId = buffer.readVarInt();
}
