#include "../../../../include/network/packet/play/KeepAlivePacketClientbound.h"

KeepAlivePacketClientbound::KeepAlivePacketClientbound(long keepAliveId)
    : keepAliveId(keepAliveId) {}

void KeepAlivePacketClientbound::write(PacketBuffer& buffer) {
    buffer.writeLong(keepAliveId);
}
