#pragma once
#include "../OutboundPacket.h"
#include "../../../game/player/Player.h" // For ItemStack

class EntityEquipmentPacket : public OutboundPacket {
public:
    EntityEquipmentPacket(int entityId, int slot, const ItemStack& item);
    int getPacketId() const override { return 0x50; } // Packet ID for Entity Equipment
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    int slot;
    ItemStack item;
};
