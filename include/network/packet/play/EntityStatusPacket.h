#pragma once

#include "../OutboundPacket.h"

class EntityStatusPacket : public OutboundPacket {
public:
    EntityStatusPacket(int entityId, char status);
    int getPacketId() const override { return 0x1A; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    char status;
};
