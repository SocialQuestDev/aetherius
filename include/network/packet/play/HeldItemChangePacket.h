#pragma once
#include "../InboundPacket.h"

class HeldItemChangePacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x25; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    short slot;
};
