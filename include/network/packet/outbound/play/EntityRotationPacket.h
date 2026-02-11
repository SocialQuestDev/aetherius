#pragma once

#include "network/packet/OutboundPacket.h"

class EntityRotationPacket : public OutboundPacket {
public:
    EntityRotationPacket(int entityId, float yaw, float pitch, bool onGround);
    int getPacketId() const override { return 0x29; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    float yaw, pitch;
    bool onGround;
};
