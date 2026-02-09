#pragma once

#include "../InboundPacket.h"

class HandshakePacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x00; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    int protocolVersion;
    std::string serverAddress;
    unsigned short serverPort;
    int nextState;
};
