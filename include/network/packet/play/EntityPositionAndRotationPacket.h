#pragma once

#include "../OutboundPacket.h"

class EntityPositionAndRotationPacket : public OutboundPacket {
public:
    EntityPositionAndRotationPacket(int entityId, short deltaX, short deltaY, short deltaZ, float yaw, float pitch, bool onGround);
    int getPacketId() const override { return 0x28; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    short deltaX, deltaY, deltaZ;
    float yaw, pitch;
    bool onGround;
};
