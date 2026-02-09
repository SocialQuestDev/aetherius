#pragma once

#include "../OutboundPacket.h"

class StatusPongPacket : public OutboundPacket {
public:
    explicit StatusPongPacket(long payload);
    int getPacketId() const override { return 0x01; }
    void write(PacketBuffer& buffer) override;

private:
    long payload;
};
