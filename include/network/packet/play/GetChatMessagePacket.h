#pragma once
#include <string>

#include "../InboundPacket.h"

class GetChatMessagePacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x03; }

    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    std::string message;
};
