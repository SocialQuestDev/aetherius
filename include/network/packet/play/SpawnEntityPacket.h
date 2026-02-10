#pragma once
#include "../OutboundPacket.h"
#include "../../../auth/UUID.h"
#include "../../../other/Vector3.h"

class SpawnEntityPacket : public OutboundPacket {
public:
    SpawnEntityPacket(int entityId, UUID objectUUID, int type, const Vector3& position,
                      float pitch, float yaw, int objectData,
                      short velocityX, short velocityY, short velocityZ);
    int getPacketId() const override { return 0x00; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    UUID objectUUID;
    int type;
    Vector3 position;
    float pitch;
    float yaw;
    int objectData;
    short velocityX;
    short velocityY;
    short velocityZ;
};
