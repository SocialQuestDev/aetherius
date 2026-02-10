#include "../../../../include/network/packet/play/EntityActionPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/network/packet/play/EntityMetadataPacket.h"
#include "../../../../include/Logger.h"

void EntityActionPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (!player) return;

    bool stateChanged = false;
    switch (actionId) {
        case 0: // Start sneaking
            player->setSneaking(true);
            stateChanged = true;
            break;
        case 1: // Stop sneaking
            player->setSneaking(false);
            stateChanged = true;
            break;
        case 3: // Start sprinting
            player->setSprinting(true);
            stateChanged = true;
            break;
        case 4: // Stop sprinting
            player->setSprinting(false);
            stateChanged = true;
            break;
        // Other actions (leaving bed, etc.) can be handled here.
    }

    if (stateChanged) {
        // Broadcast the metadata update to other players
        EntityMetadataPacket packet(*player);
        for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
            if (otherPlayer->getId() != player->getId()) {
                otherPlayer->getConnection()->send_packet(packet);
            }
        }
    }
}

void EntityActionPacket::read(PacketBuffer& buffer) {
    entityId = buffer.readVarInt();
    actionId = buffer.readVarInt();
    jumpBoost = buffer.readVarInt();
}
