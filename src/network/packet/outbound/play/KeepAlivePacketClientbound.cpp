#include "network/packet/outbound/play/KeepAlivePacketClientbound.h"

KeepAlivePacketClientbound::KeepAlivePacketClientbound(long keepAliveId)
    : keepAliveId(keepAliveId) {}

void KeepAlivePacketClientbound::write(PacketBuffer& buffer) {
    buffer.writeLong(keepAliveId);
}
