#pragma once

#include "../OutboundPacket.h"

class World; // Forward declaration

class JoinGamePacket : public OutboundPacket {
public:
    JoinGamePacket(int entityId, World& world);
    int getPacketId() const override { return 0x24; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    World& world;
};
