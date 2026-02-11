#include "network/packet/outbound/login/LoginSuccessPacket.h"

LoginSuccessPacket::LoginSuccessPacket(const UUID& uuid, const std::string& nickname) : uuid(uuid), nickname(nickname) {}

void LoginSuccessPacket::write(PacketBuffer& buffer) {
    buffer.writeUUID(uuid);
    buffer.writeString(nickname);
}
