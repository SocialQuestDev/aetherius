#include "network/packet/outbound/play/DisconnectPacketPlay.h"

DisconnectPacketPlay::DisconnectPacketPlay(const std::string& reason) : reason_(reason) {}

void DisconnectPacketPlay::write(PacketBuffer& buffer) {
    buffer.writeString(reason_);
}

int DisconnectPacketPlay::getPacketId() const {
    return 0x19;
}
