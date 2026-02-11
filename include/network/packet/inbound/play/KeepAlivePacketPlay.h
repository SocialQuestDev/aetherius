#pragma once

#include "network/packet/InboundPacket.h"

class KeepAlivePacketPlay : public InboundPacket {
public:
    int getPacketId() const override { return 0x10; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    long keepAliveId;
};
