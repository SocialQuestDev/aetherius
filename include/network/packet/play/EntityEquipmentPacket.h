#pragma once
#include "../OutboundPacket.h"
#include "../../../game/player/Player.h" // For Slot

class EntityEquipmentPacket : public OutboundPacket {
public:
    EntityEquipmentPacket(int entityId, int slot, const Slot& item);
    int getPacketId() const override { return 0x47; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    int slot;
    Slot item;
};
