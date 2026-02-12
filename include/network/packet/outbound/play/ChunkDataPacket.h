#pragma once

#include "network/packet/OutboundPacket.h"
#include "game/world/Chunk.h"

class ChunkDataPacket : public OutboundPacket {
public:
    explicit ChunkDataPacket(ChunkColumn* chunk);
    void write(PacketBuffer& buffer) override;
    int getPacketId() const override;

private:
    ChunkColumn* chunk_;
};
