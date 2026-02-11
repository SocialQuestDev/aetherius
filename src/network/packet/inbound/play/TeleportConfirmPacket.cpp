#include "network/packet/inbound/play/TeleportConfirmPacket.h"
#include "network/Connection.h"
#include "Server.h"
#include "game/world/World.h"
#include "game/world/Chunk.h"
#include "game/player/PlayerList.h"
#include <cmath>
#include <boost/asio.hpp>

void TeleportConfirmPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (!player) return;

    // The player has confirmed the teleport. Now we can send the chunk data.
    // This should be done asynchronously to avoid blocking the main thread.

    Vector3 pos = player->getPosition();
    int chunkX = static_cast<int>(std::floor(pos.x / 16.0));
    int chunkZ = static_cast<int>(std::floor(pos.z / 16.0));

    // Send Update View Position packet. This tells the client where to center its view.
    PacketBuffer vp;
    vp.writeVarInt(0x40); // Packet ID for Update View Position
    vp.writeVarInt(chunkX);
    vp.writeVarInt(chunkZ);
    connection.send_packet(vp);

    auto& io_context = Server::get_instance().get_io_context();
    auto conn_ptr = connection.shared_from_this();
    int viewDistance = player->getViewDistance();

    // Update last known chunk position for the connection
    connection.last_chunk_x_ = chunkX;
    connection.last_chunk_z_ = chunkZ;

    // Post a task to the io_context's thread pool to handle chunk loading and sending.
    boost::asio::post(io_context, [conn_ptr, chunkX, chunkZ, viewDistance]() {
        Server& server = Server::get_instance();
        World& world = server.get_world();

        // Collect chunks that need to be generated
        std::vector<std::pair<int, int>> chunks_to_load;

        for (int x = -viewDistance; x <= viewDistance; ++x) {
            for (int z = -viewDistance; z <= viewDistance; ++z) {
                int cx = chunkX + x;
                int cz = chunkZ + z;

                // Check if chunk already exists
                ChunkColumn* existing = world.getChunk(cx, cz);
                if (existing) {
                    // Chunk already exists, just serialize and send
                    auto serialized_chunk = std::make_shared<std::vector<uint8_t>>(existing->serialize());
                    boost::asio::post(conn_ptr->get_write_strand(), [conn_ptr, serialized_chunk]() {
                        PacketBuffer cp;
                        cp.writeVarInt(0x20);
                        cp.data.insert(cp.data.end(), serialized_chunk->begin(), serialized_chunk->end());
                        conn_ptr->send_packet(cp);
                    });
                } else {
                    // Need to generate this chunk
                    chunks_to_load.push_back({cx, cz});
                }
            }
        }

        // Generate all new chunks in parallel
        std::vector<std::future<ChunkColumn*>> futures;
        futures.reserve(chunks_to_load.size());

        for (const auto& [cx, cz] : chunks_to_load) {
            futures.push_back(world.generateChunkAsync(cx, cz));
        }

        // Wait for chunks to be generated and send them
        for (auto& future : futures) {
            try {
                ChunkColumn* chunk = future.get();
                if (chunk) {
                    auto serialized_chunk = std::make_shared<std::vector<uint8_t>>(chunk->serialize());
                    boost::asio::post(conn_ptr->get_write_strand(), [conn_ptr, serialized_chunk]() {
                        PacketBuffer cp;
                        cp.writeVarInt(0x20);
                        cp.data.insert(cp.data.end(), serialized_chunk->begin(), serialized_chunk->end());
                        conn_ptr->send_packet(cp);
                    });
                }
            } catch (const std::exception& e) {
                // Log error but continue with other chunks
            }
        }
    });
}

void TeleportConfirmPacket::read(PacketBuffer& buffer) {
    teleportId = buffer.readVarInt();
}
