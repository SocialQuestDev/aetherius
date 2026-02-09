#include "../../../../include/network/packet/play/TeleportConfirmPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/Server.h"
#include "../../../../include/game/world/World.h"
#include "../../../../include/game/world/Chunk.h"

void TeleportConfirmPacket::handle(Connection& connection) {
    // packet_update_view_position: 0x41
    PacketBuffer vp;
    vp.writeVarInt(0x40);
    vp.writeVarInt(0);
    vp.writeVarInt(0);
    connection.send_packet(vp);

    Server& server = Server::get_instance();
    World& world = server.get_world();
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            ChunkColumn* chunk = world.generateChunk(x, z);
            PacketBuffer cp;
            // packet_map_chunk: 0x21
            cp.writeVarInt(0x20);
            auto p = chunk->serialize();
            cp.data.insert(cp.data.end(), p.begin(), p.end());
            connection.send_packet(cp);
        }
    }
}

void TeleportConfirmPacket::read(PacketBuffer& buffer) {
    teleportId = buffer.readVarInt();
}
