#pragma once
#include "../OutboundPacket.h"

class EntityAnimationPacket : public OutboundPacket {
public:
    EntityAnimationPacket(int entityId, int animation);
    int getPacketId() const override { return 0x05; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    int animation;
};
