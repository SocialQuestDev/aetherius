#include "../../../../include/network/packet/play/BlockDigRequestPacket.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/network/packet/play/BlockDigResponsePacket.h"

void BlockDigRequestPacket::handle(Connection& connection) {
    std::shared_ptr<Player> digger = connection.getPlayer();
    if (!digger) {
        return;
    }

    // Determine the destroy stage based on the dig status.
    // For a full implementation, the server should track breaking progress over time.
    char destroyStage = -2; // Default to no action

    switch (status) {
        case 0: // Started digging
            destroyStage = 0; // Show first stage of breaking
            break;
        case 1: // Cancelled digging
            destroyStage = -1; // Remove breaking animation
            break;
        case 2: // Finished digging
            // For now, just clear the animation.
            // The actual block destruction logic was causing issues and has been removed.
            destroyStage = -1;
            break;
    }

    if (destroyStage != -2) {
        BlockDigResponsePacket packet(digger->getId(), this->position, destroyStage);

        // Broadcast the animation to all players except the one digging.
        for (const auto& player : PlayerList::getInstance().getPlayers()) {
            if (player->getId() != digger->getId()) {
                player->getConnection()->send_packet(packet);
            }
        }
    }
}

void BlockDigRequestPacket::read(PacketBuffer& buffer) {
    status = buffer.readVarInt();
    long long packed_pos = buffer.readLong();
    position.x = (float)(int)(packed_pos >> 38);
    position.y = (float)(int)(packed_pos & 0xFFF);
    position.z = (float)(int)((packed_pos >> 12) & 0x3FFFFFF);

    // Convert from 2's complement
    if (position.x >= 33554432) position.x -= 67108864;
    if (position.y >= 2048)     position.y -= 4096;
    if (position.z >= 33554432) position.z -= 67108864;

    face = buffer.readByte();
}
