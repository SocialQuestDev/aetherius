#pragma once
#include "network/packet/OutboundPacket.h"

class UpdateHealthPacket : public OutboundPacket {
public:
    UpdateHealthPacket(float health, int food, float saturation);
    int getPacketId() const override { return 0x49; }
    void write(PacketBuffer& buffer) override;

private:
    float health;
    int food;
    float saturation;
};
