#pragma once
#include "network/packet/InboundPacket.h"

class ArmAnimationPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x2C; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;
};
