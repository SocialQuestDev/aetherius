#pragma once

#include "network/packet/OutboundPacket.h"

class EntityStatusPacket : public OutboundPacket {
public:
    EntityStatusPacket(int entityId, std::byte status);
    int getPacketId() const override { return 0x1A; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    std::byte status;
};
