#include "network/packet/outbound/play/EntityEquipmentPacket.h"
#include "game/player/Player.h"

EntityEquipmentPacket::EntityEquipmentPacket(int entityId, EquipmentSlot slot, const Slot& item)
    : entityId_(entityId) {
    equipment_.emplace_back(slot, item);
}

EntityEquipmentPacket::EntityEquipmentPacket(int entityId, const std::vector<std::pair<EquipmentSlot, Slot>>& equipment)
    : entityId_(entityId), equipment_(equipment) {}

void EntityEquipmentPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId_);
    for (size_t i = 0; i < equipment_.size(); ++i) {
        auto& [slot, item] = equipment_[i];
        bool isLast = (i == equipment_.size() - 1);

        uint8_t slotByte = static_cast<uint8_t>(slot);
        if (!isLast) {
            slotByte |= 0x80;
        }

        buffer.writeByte(slotByte);
        buffer.writeSlot(item);
    }
}

int EntityEquipmentPacket::getPacketId() const {
    return 0x50;
}
