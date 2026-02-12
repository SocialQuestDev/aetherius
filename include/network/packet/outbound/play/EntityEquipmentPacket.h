#pragma once

#include "network/packet/OutboundPacket.h"
#include <vector>

struct Slot;

enum class EquipmentSlot {
    MAIN_HAND = 0,
    OFF_HAND = 1,
    BOOTS = 2,
    LEGGINGS = 3,
    CHESTPLATE = 4,
    HELMET = 5
};

class EntityEquipmentPacket : public OutboundPacket {
public:
    EntityEquipmentPacket(int entityId, EquipmentSlot slot, const Slot& item);
    EntityEquipmentPacket(int entityId, const std::vector<std::pair<EquipmentSlot, Slot>>& equipment);

    void write(PacketBuffer& buffer) override;
    int getPacketId() const override;

private:
    int entityId_;
    std::vector<std::pair<EquipmentSlot, Slot>> equipment_;
};
