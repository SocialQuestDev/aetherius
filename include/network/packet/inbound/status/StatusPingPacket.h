#pragma once

#include "network/packet/InboundPacket.h"

class StatusPingPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x01; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    long payload;
};
