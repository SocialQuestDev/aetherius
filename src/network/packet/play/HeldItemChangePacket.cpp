#include "../../../../include/network/packet/play/HeldItemChangePacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/network/packet/play/EntityEquipmentPacket.h"

void HeldItemChangePacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setHeldItemSlot(slot);

        // Broadcast the change to other players
        // The slot for the main hand is 0
        EntityEquipmentPacket packet(player->getId(), 0, player->getInventory()[36 + player->getHeldItemSlot()]);
        for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
            if (otherPlayer->getId() != player->getId()) {
                otherPlayer->getConnection()->send_packet(packet);
            }
        }
    }
}

void HeldItemChangePacket::read(PacketBuffer& buffer) {
    slot = buffer.readShort();
}
