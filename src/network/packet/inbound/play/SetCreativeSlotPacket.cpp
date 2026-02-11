#include "network/packet/inbound/play/SetCreativeSlotPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "network/PacketBuffer.h"

void SetCreativeSlotPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setInventorySlot(slot, item);
    }
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
