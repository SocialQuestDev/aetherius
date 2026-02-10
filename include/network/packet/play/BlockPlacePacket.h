#pragma once
#include "../InboundPacket.h"
#include "../../../other/Vector3.h"

class BlockPlacePacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x2E; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    int hand;
    Vector3 position;
    int face;
    float cursorX, cursorY, cursorZ;
    bool insideBlock;
};
