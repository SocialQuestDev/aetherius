#pragma once
#include "../InboundPacket.h"

class ClientAbilitiesPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x1A; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    uint8_t flags;
};
