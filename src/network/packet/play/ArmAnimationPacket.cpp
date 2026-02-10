#include "../../../../include/network/packet/play/ArmAnimationPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/network/packet/play/EntityAnimationPacket.h"

void ArmAnimationPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        // Broadcast the animation to other players
        EntityAnimationPacket packet(player->getId(), 0); // 0 for swing main arm
        for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
            if (otherPlayer->getId() != player->getId()) {
                otherPlayer->getConnection()->send_packet(packet);
            }
        }
    }
}

void ArmAnimationPacket::read(PacketBuffer& buffer) {
    // This packet has no fields
}
