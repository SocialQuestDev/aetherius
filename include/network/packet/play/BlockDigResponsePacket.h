#pragma once
#include "../OutboundPacket.h"
#include "../../../other/Vector3.h"

class BlockDigResponsePacket : public OutboundPacket {
public:
    BlockDigResponsePacket(int entityId, const Vector3& position, char destroyStage);
    int getPacketId() const override { return 0x09; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    Vector3 position;
    char destroyStage;
};
