#include "../../../../include/network/packet/play/EntityEquipmentPacket.h"
#include "../../../../include/network/PacketBuffer.h"

EntityEquipmentPacket::EntityEquipmentPacket(int entityId, int slot, const Slot& item)
    : entityId(entityId), slot(slot), item(item) {}

void EntityEquipmentPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    buffer.writeVarInt(slot);
    // For now, we'll just write a present byte and the item ID and count.
    // A full implementation would require writing NBT data for the item.
    buffer.writeBoolean(item.itemId != 0);
    if (item.itemId != 0) {
        buffer.writeVarInt(item.itemId);
        buffer.writeByte(item.count);
        buffer.writeByte(0); // No NBT data
    }
}
