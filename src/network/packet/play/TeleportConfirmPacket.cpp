#include "../../../../include/network/packet/play/TeleportConfirmPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/Server.h"
#include "../../../../include/game/world/World.h"
#include "../../../../include/game/world/Chunk.h"
#include "../../../../include/game/player/PlayerList.h"

void TeleportConfirmPacket::handle(Connection& connection) {
    PacketBuffer vp;
    vp.writeVarInt(0x40);
    vp.writeVarInt(0);
    vp.writeVarInt(0);
    connection.send_packet(vp);

    Server& server = Server::get_instance();
    World& world = server.get_world();
    for (int x = -6; x <= 6; ++x) {
        for (int z = -6; z <= 6; ++z) {
            ChunkColumn* chunk = world.generateChunk(x, z);
            PacketBuffer cp;
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
