#pragma once

#include "network/packet/OutboundPacket.h"

class TimeUpdatePacket : public OutboundPacket {
public:
    TimeUpdatePacket(int64_t worldAge, int64_t timeOfDay);
    int getPacketId() const override { return 0x4E; }
    void write(PacketBuffer& buffer) override;

private:
    int64_t worldAge;
    int64_t timeOfDay;
};
