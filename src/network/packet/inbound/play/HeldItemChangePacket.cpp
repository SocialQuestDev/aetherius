#include "network/packet/inbound/play/HeldItemChangePacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/EntityEquipmentPacket.h"

void HeldItemChangePacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setHeldItemSlot(slot);

        const auto& inventory = player->getInventory();
        int inventoryIndex = 36 + player->getHeldItemSlot();

        Slot heldItem;
        if (inventoryIndex < inventory.size()) {
            heldItem = inventory[inventoryIndex];
        }

        EntityEquipmentPacket packet(player->getId(), EquipmentSlot::MAIN_HAND, heldItem);
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
