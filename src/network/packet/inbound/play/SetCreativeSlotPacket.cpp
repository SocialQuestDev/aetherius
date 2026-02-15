#include "network/packet/inbound/play/SetCreativeSlotPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "network/PacketBuffer.h"
#include "Server.h"

void SetCreativeSlotPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const short slot_index = slot;
    const Slot item_copy = item;
    Server::get_instance().post_game_task([self, slot_index, item_copy]() {
        auto player = self->getPlayer();
        if (player) {
            player->setInventorySlot(slot_index, item_copy);
        }
    });
}

void SetCreativeSlotPacket::read(PacketBuffer& buffer) {
    slot = buffer.readShort();
    bool present = buffer.readBoolean();
    if (present) {
        item.itemId = buffer.readVarInt();
        item.count = buffer.readByte();
        buffer.readNbt(); // Skip NBT data
    } else {
        item.itemId = 0;
        item.count = 0;
    }
}
