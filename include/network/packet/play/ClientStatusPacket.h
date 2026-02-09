#pragma once

#include "../InboundPacket.h"

class ClientStatusPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x04; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    int actionId;
};
