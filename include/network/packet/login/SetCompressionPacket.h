#pragma once

#include "../OutboundPacket.h"

class SetCompressionPacket : public OutboundPacket {
public:
    explicit SetCompressionPacket(int threshold);
    int getPacketId() const override { return 0x03; }
    void write(PacketBuffer& buffer) override;

private:
    int threshold;
};
