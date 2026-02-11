#pragma once
#include <string>

#include "network/packet/InboundPacket.h"
#include "other/Vector3.h"

class BlockDigRequestPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x1b; }

    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    int status;
    Vector3 position;
    uint8_t face;
};
