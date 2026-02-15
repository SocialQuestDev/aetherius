#include "network/packet/inbound/play/ArmAnimationPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/EntityAnimationPacket.h"
#include "Server.h"

void ArmAnimationPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    Server::get_instance().post_game_task([self]() {
        auto player = self->getPlayer();
        if (player) {
            EntityAnimationPacket packet(player->getId(), 0);
            for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
                if (otherPlayer->getId() != player->getId()) {
                    otherPlayer->getConnection()->send_packet(packet);
                }
            }
        }
    });
}

void ArmAnimationPacket::read(PacketBuffer& buffer) {
}
