#include "network/packet/outbound/status/StatusResponsePacket.h"

StatusResponsePacket::StatusResponsePacket(std::string jsonResponse) : jsonResponse(std::move(jsonResponse)) {}

void StatusResponsePacket::write(PacketBuffer& buffer) {
    buffer.writeString(jsonResponse);
}
