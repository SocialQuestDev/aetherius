#pragma once

#include "network/packet/OutboundPacket.h"

class EntityHeadLookPacket : public OutboundPacket {
public:
    EntityHeadLookPacket(int entityId, float headYaw);
    int getPacketId() const override { return 0x3a; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    float headYaw;
};
