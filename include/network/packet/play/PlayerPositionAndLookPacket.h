#pragma once

#include "../OutboundPacket.h"

class PlayerPositionAndLookPacket : public OutboundPacket {
public:
    PlayerPositionAndLookPacket(double x, double y, double z, float yaw, float pitch, char flags, int teleportId);
    int getPacketId() const override { return 0x34; }
    void write(PacketBuffer& buffer) override;

private:
    double x, y, z;
    float yaw, pitch;
    char flags;
    int teleportId;
};
