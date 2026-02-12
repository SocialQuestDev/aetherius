#include "network/packet/outbound/play/ChunkDataPacket.h"
#include "game/nbt/NbtBuilder.h"

ChunkDataPacket::ChunkDataPacket(ChunkColumn* chunk) : chunk_(chunk) {}

void ChunkDataPacket::write(PacketBuffer& buffer) {
    buffer.writeInt(chunk_->getX());
    buffer.writeInt(chunk_->getZ());
    buffer.writeBoolean(true); // Full chunk

    int primaryBitmask = 0;
    for (int i = 0; i < 16; ++i) {
        if (chunk_->getSection(i) != nullptr && !chunk_->getSection(i)->isEmpty()) {
            primaryBitmask |= (1 << i);
        }
    }
    buffer.writeVarInt(primaryBitmask);

    NbtBuilder heightmapNbt;
    heightmapNbt.startCompound();
    {
        heightmapNbt.startCompound("MOTION_BLOCKING");
        std::vector<int64_t> motion_blocking(37, 0);
        heightmapNbt.writeLongArray("MOTION_BLOCKING", motion_blocking);
        heightmapNbt.endCompound();
    }
    heightmapNbt.endCompound();

    buffer.writeNbt(heightmapNbt.buffer);

    buffer.writeVarInt(1024);
    for (int i = 0; i < 1024; ++i) {
        buffer.writeVarInt(1);
    }

    PacketBuffer chunkDataBuffer;
    for (int i = 0; i < 16; ++i) {
        if ((primaryBitmask >> i) & 1) {
            chunk_->getSection(i)->writeToNetwork(chunkDataBuffer);
        }
    }

    buffer.writeVarInt(chunkDataBuffer.data.size());
    buffer.data.insert(buffer.data.end(), chunkDataBuffer.data.begin(), chunkDataBuffer.data.end());

    buffer.writeVarInt(0);
}

int ChunkDataPacket::getPacketId() const {
    return 0x20;
}
