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

    // Static heightmap NBT to avoid rebuilding every time
    static const std::vector<uint8_t> heightmap_nbt = []() {
        NbtBuilder heightmapNbt;
        heightmapNbt.startCompound();
        {
            heightmapNbt.startCompound("MOTION_BLOCKING");
            std::vector<int64_t> motion_blocking(37, 0);
            heightmapNbt.writeLongArray("MOTION_BLOCKING", motion_blocking);
            heightmapNbt.endCompound();
        }
        heightmapNbt.endCompound();
        return heightmapNbt.buffer;
    }();

    buffer.writeNbt(heightmap_nbt);

    // Static biome data to avoid rebuilding every time
    buffer.writeVarInt(1024);
    static const std::vector<int> biome_data(1024, 1);
    for (int biome : biome_data) {
        buffer.writeVarInt(biome);
    }

    // Use cached chunk data
    auto cachedData = chunk_->getCachedNetworkData();
    buffer.writeVarInt(cachedData.size());
    buffer.data.insert(buffer.data.end(), cachedData.begin(), cachedData.end());

    buffer.writeVarInt(0);
}

int ChunkDataPacket::getPacketId() const {
    return 0x20;
}
