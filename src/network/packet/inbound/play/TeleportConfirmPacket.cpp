#include "network/packet/inbound/play/TeleportConfirmPacket.h"
#include "network/Connection.h"
#include "Server.h"
#include "game/world/World.h"
#include "game/world/Chunk.h"
#include "game/player/PlayerList.h"
#include <cmath>

void TeleportConfirmPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (!player) return;

    // Get player's chunk coordinates
    Vector3 pos = player->getPosition();
    int chunkX = static_cast<int>(std::floor(pos.x / 16.0));
    int chunkZ = static_cast<int>(std::floor(pos.z / 16.0));

    // Send ViewPosition packet with player's chunk coordinates
    PacketBuffer vp;
    vp.writeVarInt(0x40);
    vp.writeVarInt(chunkX);
    vp.writeVarInt(chunkZ);
    connection.send_packet(vp);

    Server& server = Server::get_instance();
    World& world = server.get_world();
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            ChunkColumn* chunk = world.generateChunk(chunkX + x, chunkZ + z);
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
