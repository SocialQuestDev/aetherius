#include "../../../../include/network/packet/play/ClientAbilitiesPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"

void ClientAbilitiesPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        bool isFlying = (flags & 0x02) != 0;
        player->setFlying(isFlying);
    }
}

void ClientAbilitiesPacket::read(PacketBuffer& buffer) {
    flags = buffer.readByte();
    // The flying speed and FOV modifier fields were removed in this protocol version.
}
