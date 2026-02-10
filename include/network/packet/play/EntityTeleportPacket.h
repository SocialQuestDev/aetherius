#pragma once
#include "../OutboundPacket.h"
#include "../../../other/Vector3.h"

class EntityTeleportPacket : public OutboundPacket {
public:
    EntityTeleportPacket(int entityId, const Vector3& position, float yaw, float pitch, bool onGround);
    int getPacketId() const override { return 0x56; } // Packet ID for Entity Teleport
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    Vector3 position;
    float yaw;
    float pitch;
    bool onGround;
};
