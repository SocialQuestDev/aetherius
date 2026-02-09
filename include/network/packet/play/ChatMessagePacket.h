#pragma once

#include "../OutboundPacket.h"
#include "../../../auth/UUID.h"
#include <string>

class ChatMessagePacket : public OutboundPacket {
public:
    ChatMessagePacket(const std::string& message, char position, const UUID& sender);
    int getPacketId() const override { return 0x0E; }
    void write(PacketBuffer& buffer) override;

private:
    std::string message;
    char position;
    UUID sender;
};
