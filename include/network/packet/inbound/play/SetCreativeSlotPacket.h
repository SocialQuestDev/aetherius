#pragma once
#include "network/packet/InboundPacket.h"
#include "game/player/Player.h" // For Slot

class SetCreativeSlotPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x28; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    short slot;
    Slot item;
};
