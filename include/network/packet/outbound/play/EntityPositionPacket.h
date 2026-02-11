#pragma once

#include "network/packet/OutboundPacket.h"

class EntityPositionPacket : public OutboundPacket {
public:
    explicit EntityPositionPacket(int entityId);
    int getPacketId() const override { return 0x2A; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
};
