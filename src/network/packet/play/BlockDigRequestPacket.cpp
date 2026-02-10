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

    char destroyStage = -2;

    switch (status) {
        case 0: // Started digging
            // Show the first stage of the breaking animation to other clients.
            // For a full implementation, the server should track breaking progress over time.
            destroyStage = 0;
            break;
        case 1: // Cancelled digging
            // Remove the breaking animation from other clients.
            destroyStage = -1;
            break;
        case 2: // Finished digging
            // The block is broken. Clear the animation on other clients.
            // TODO: Handle the actual block breaking logic here:
            // 1. Update the world state (e.g., world.setBlock(position, Blocks.AIR)).
            // 2. Send a BlockChangePacket to all players to show the block is gone.
            destroyStage = -1;
            break;
    }

    if (destroyStage != -2) {
        BlockDigResponsePacket packet(digger->getId(), this->position, destroyStage);

        for (const auto& player : PlayerList::getInstance().getPlayers()) {
            if (player->getId() != digger->getId()) {
                player->getConnection()->send_packet(packet);
            }
        }
    }
}

void BlockDigRequestPacket::read(PacketBuffer& buffer) {
    status = buffer.readVarInt();
    position = buffer.readPosition();
    face = buffer.readByte();
}
