#include "network/packet/inbound/play/ClientAbilitiesPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "Server.h"

void ClientAbilitiesPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const uint8_t local_flags = flags;
    Server::get_instance().post_game_task([self, local_flags]() {
        auto player = self->getPlayer();
        if (player) {
            bool isFlying = (local_flags & 0x02) != 0;
            player->setFlying(isFlying);
        }
    });
}

void ClientAbilitiesPacket::read(PacketBuffer& buffer) {
    flags = buffer.readByte();
}
