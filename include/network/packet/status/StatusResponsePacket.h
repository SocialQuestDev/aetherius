#pragma once
#include "../OutboundPacket.h"
#include <string>

class StatusResponsePacket : public OutboundPacket {
public:
    explicit StatusResponsePacket(std::string jsonResponse);
    int getPacketId() const override { return 0x00; }
    void write(PacketBuffer& buffer) override;

private:
    std::string jsonResponse;
};
