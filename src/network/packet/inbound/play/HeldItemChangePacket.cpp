#include "network/packet/inbound/play/HeldItemChangePacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/EntityEquipmentPacket.h"
#include "Server.h"

void HeldItemChangePacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const short new_slot = slot;
    Server::get_instance().post_game_task([self, new_slot]() {
        auto player = self->getPlayer();
        if (player) {
            player->setHeldItemSlot(new_slot);

            const auto& inventory = player->getInventory();
            int inventoryIndex = 36 + player->getHeldItemSlot();

            Slot heldItem;
            if (inventoryIndex < static_cast<int>(inventory.size())) {
                heldItem = inventory[inventoryIndex];
            }

            EntityEquipmentPacket packet(player->getId(), EquipmentSlot::MAIN_HAND, heldItem);
            for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
                if (otherPlayer->getId() != player->getId()) {
                    otherPlayer->getConnection()->send_packet(packet);
                }
            }
        }
    });
}

void HeldItemChangePacket::read(PacketBuffer& buffer) {
    slot = buffer.readShort();
}
