#include "network/packet/outbound/login/DisconnectPacket.h"

DisconnectPacket::DisconnectPacket(const std::string& reason) : reason_(reason) {}

void DisconnectPacket::write(PacketBuffer& buffer) {
    buffer.writeString(reason_);
}

int DisconnectPacket::getPacketId() const {
    return 0x00;
}
