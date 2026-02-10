#pragma once
#include "../OutboundPacket.h"
#include "../../../other/Vector3.h"

class BlockChangePacket : public OutboundPacket {
public:
    BlockChangePacket(const Vector3& position, int blockId);
    int getPacketId() const override { return 0x0b; }
    void write(PacketBuffer& buffer) override;

private:
    Vector3 position;
    int blockId;
};
