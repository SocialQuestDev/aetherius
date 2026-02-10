#pragma once
#include "../InboundPacket.h"
#include "../../../game/player/Player.h" // For ItemStack

class SetCreativeSlotPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x28; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    short slot;
    ItemStack item;
};
