#pragma once
#include "../OutboundPacket.h"

class EntityAnimationPacket : public OutboundPacket {
public:
    EntityAnimationPacket(int entityId, int animation);
    int getPacketId() const override { return 0x06; } // Packet ID for Entity Animation
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    int animation;
};
