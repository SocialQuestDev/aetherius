#include "network/packet/inbound/play/TeleportConfirmPacket.h"
#include "network/Connection.h"
#include "Server.h"
#include "game/world/World.h"
#include "game/world/Chunk.h"
#include "game/world/ChunkManager.h"
#include "game/player/Player.h"
#include <cmath>

void TeleportConfirmPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    Server::get_instance().post_game_task([self]() {
        auto player = self->getPlayer();
        if (!player) return;

        Vector3 pos = player->getPosition();
        int chunkX = static_cast<int>(std::floor(pos.x / 16.0));
        int chunkZ = static_cast<int>(std::floor(pos.z / 16.0));

        PacketBuffer vp;
        vp.writeVarInt(0x40);
        vp.writeVarInt(chunkX);
        vp.writeVarInt(chunkZ);
        self->send_packet(vp);

        self->update_chunks();
    });
}

void TeleportConfirmPacket::read(PacketBuffer& buffer) {
    teleportId = buffer.readVarInt();
}
