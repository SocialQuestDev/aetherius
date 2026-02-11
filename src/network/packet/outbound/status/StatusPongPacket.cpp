#include "network/packet/outbound/status/StatusPongPacket.h"

StatusPongPacket::StatusPongPacket(long payload) : payload(payload) {}

void StatusPongPacket::write(PacketBuffer& buffer) {
    buffer.writeLong(payload);
}
