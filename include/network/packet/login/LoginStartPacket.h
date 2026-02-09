#pragma once

#include "../InboundPacket.h"
#include <string>

class LoginStartPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x00; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    std::string nickname;
};
