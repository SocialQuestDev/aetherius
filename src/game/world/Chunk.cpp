#include "game/world/Chunk.h"

int ChunkColumn::getBlock(int x, int y, int z) const {
    if (y < 0 || y >= 256) return 0; // Air
    int sectionIndex = y / 16;
    int blockIndex = (y % 16) * 16 * 16 + z * 16 + x;
    return sections[sectionIndex].blocks[blockIndex];
}

void ChunkColumn::setBlock(int x, int y, int z, int blockId) {
    if (y < 0 || y >= 256) return;
    int sectionIndex = y / 16;
    int blockIndex = (y % 16) * 16 * 16 + z * 16 + x;
    sections[sectionIndex].blocks[blockIndex] = blockId;
}

std::vector<uint8_t> ChunkColumn::serialize() {
    PacketBuffer buf;

    buf.writeInt(x);
    buf.writeInt(z);
    buf.writeBoolean(true);

    int mask = 0;
    for (int i = 0; i < 16; ++i) {
        bool hasBlocks = false;
        for(int b : sections[i].blocks) if(b != 0) { hasBlocks = true; break; }
        if (hasBlocks) mask |= (1 << i);
    }
    buf.writeVarInt(mask);

    NbtBuilder heightmapNbt;
    heightmapNbt.startCompound();
    std::vector<int64_t> motionBlocking(37, 0);
    heightmapNbt.writeLongArray("MOTION_BLOCKING", motionBlocking);
    heightmapNbt.endCompound();
    buf.writeNbt(heightmapNbt.buffer);

    buf.writeVarInt(1024);
    for(int i = 0; i < 1024; ++i) {
        buf.writeVarInt(1); // Plains biome
    }

    PacketBuffer sectionsData;
    for (int i = 0; i < 16; ++i) {
        if (mask & (1 << i)) {
            auto s = sections[i].serialize();
            sectionsData.data.insert(sectionsData.data.end(), s.begin(), s.end());
        }
    }

    buf.writeVarInt((int)sectionsData.data.size());
    buf.data.insert(buf.data.end(), sectionsData.data.begin(), sectionsData.data.end());

    buf.writeVarInt(0); // Block Entities

    return buf.data;
}

// This is for saving to disk, which can be simpler than network serialization.
std::vector<uint8_t> ChunkColumn::serializeForFile() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(16 * 4096); // Reserve space for 16 sections of 4096 blocks (1 byte per block)
    for (const auto& section : sections) {
        for (int blockId : section.blocks) {
            // For simplicity, we're still writing 1 byte per block.
            // A more advanced format might use palettes here too.
            buffer.push_back(static_cast<uint8_t>(blockId));
        }
    }
    return buffer;
}

void ChunkColumn::deserialize(const std::vector<uint8_t>& data) {
    int index = 0;
    for (auto& section : sections) {
        for (int& blockId : section.blocks) {
            if (index < data.size()) {
                blockId = data[index++];
            } else {
                blockId = 0; // Air
            }
        }
    }
}
