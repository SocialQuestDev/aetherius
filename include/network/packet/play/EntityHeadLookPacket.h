#pragma once

#include "../OutboundPacket.h"

class EntityHeadLookPacket : public OutboundPacket {
public:
    EntityHeadLookPacket(int entityId, float headYaw, float headPitch);
    int getPacketId() const override { return 0x3E; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    float headYaw;
    float headPitch;
};
