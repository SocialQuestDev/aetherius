#pragma once

#include "network/packet/OutboundPacket.h"

class TimeUpdatePacket : public OutboundPacket {
public:
    TimeUpdatePacket(int64_t worldAge, int64_t timeOfDay);
    int getPacketId() const override { return 0x4E; }
    void write(PacketBuffer& buffer) override;

private:
    int64_t worldAge;    // Total ticks since world creation
    int64_t timeOfDay;   // Current time of day (0-24000, or negative to stop sun movement)
};
