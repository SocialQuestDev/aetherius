#include "network/packet/inbound/play/ArmAnimationPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/EntityAnimationPacket.h"

void ArmAnimationPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        EntityAnimationPacket packet(player->getId(), 0);
        for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
            if (otherPlayer->getId() != player->getId()) {
                otherPlayer->getConnection()->send_packet(packet);
            }
        }
    }
}

void ArmAnimationPacket::read(PacketBuffer& buffer) {
}
