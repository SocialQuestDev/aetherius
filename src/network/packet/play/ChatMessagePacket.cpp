#include "../../../../include/network/packet/play/ChatMessagePacket.h"

ChatMessagePacket::ChatMessagePacket(const std::string& message, char position, const UUID& sender)
    : message(message), position(position), sender(sender) {}

void ChatMessagePacket::write(PacketBuffer& buffer) {
    buffer.writeString(message);
    buffer.writeByte(position);
    buffer.writeUUID(sender);
}
