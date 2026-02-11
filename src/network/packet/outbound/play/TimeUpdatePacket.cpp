#include "network/packet/outbound/play/TimeUpdatePacket.h"
#include "network/PacketBuffer.h"

TimeUpdatePacket::TimeUpdatePacket(int64_t worldAge, int64_t timeOfDay)
    : worldAge(worldAge), timeOfDay(timeOfDay) {}

void TimeUpdatePacket::write(PacketBuffer& buffer) {
    buffer.writeLong(worldAge);
    buffer.writeLong(timeOfDay);
}
