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

    BlockDigResponsePacket packet(digger->getId(), this->position, destroyStage);
    for (const auto& player : PlayerList::getInstance().getPlayers()) {
        if (player->getId() != digger->getId()) {
            player->getConnection()->send_packet(packet);
        }
    }
}

void BlockDigRequestPacket::read(PacketBuffer& buffer) {
    status = buffer.readVarInt();
    position = buffer.readPosition();
    face = buffer.readByte();
}
