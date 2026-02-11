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

    // Post a task to the io_context's thread pool to handle chunk loading and sending.
    boost::asio::post(io_context, [conn_ptr, chunkX, chunkZ]() {
        Server& server = Server::get_instance();
        World& world = server.get_world();
        int viewDistance = 2; // Example view distance

        for (int x = -viewDistance; x <= viewDistance; ++x) {
            for (int z = -viewDistance; z <= viewDistance; ++z) {
                // Generate or load the chunk. This is the heavy part.
                ChunkColumn* chunk = world.generateChunk(chunkX + x, chunkZ + z);
                if (!chunk) continue;

                // Serialize the chunk data. This can also be heavy.
                // Use a shared_ptr to manage the lifetime of the serialized data across async calls.
                auto serialized_chunk = std::make_shared<std::vector<uint8_t>>(chunk->serialize());

                // Post the packet sending to the connection's write strand to ensure thread safety.
                boost::asio::post(conn_ptr->get_write_strand(), [conn_ptr, serialized_chunk]() {
                    PacketBuffer cp;
                    cp.writeVarInt(0x20); // Packet ID for Chunk Data
                    cp.data.insert(cp.data.end(), serialized_chunk->begin(), serialized_chunk->end());
                    conn_ptr->send_packet(cp);
                });
            }
        }
    });
}

void TeleportConfirmPacket::read(PacketBuffer& buffer) {
    teleportId = buffer.readVarInt();
}
