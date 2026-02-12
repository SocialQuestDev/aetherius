#include "game/world/Chunk.h"
#include <numeric>

ChunkColumn::ChunkColumn(int x, int z) : x(x), z(z) {
    for (auto& section : sections) {
        section = std::make_unique<ChunkSection>();
    }
}

void ChunkColumn::touch() {
    last_accessed = std::chrono::steady_clock::now();
}

bool ChunkSection::isEmpty() const {
    for (int blockId : blocks) {
        if (blockId != 0) return false;
    }
    return true;
}

void ChunkSection::writeToNetwork(PacketBuffer& buffer) {
    short blockCount = 0;
    std::map<int, int> palette;
    palette.emplace(0, 0);

    for (int blockId : blocks) {
        if (blockId != 0) {
            blockCount++;
        }
        palette.emplace(blockId, 0);
    }
    buffer.writeShort(blockCount);

    uint8_t bitsPerBlock = std::max(4, (int)ceil(log2(palette.size())));
    if (bitsPerBlock > 8) {
        bitsPerBlock = 15;
    }
    buffer.writeByte(bitsPerBlock);

    if (bitsPerBlock != 15) {
        buffer.writeVarInt(palette.size());
        int paletteIndex = 0;
        for (auto const& [blockId, val] : palette) {
            palette[blockId] = paletteIndex++;
            buffer.writeVarInt(blockId);
        }
    }

    const int longs_size = (4096 * bitsPerBlock + 63) / 64;
    buffer.writeVarInt(longs_size);

    std::array<uint64_t, 512> data{};
    if (longs_size > data.size()) {
        throw std::runtime_error("Calculated long array size is too large!");
    }

    for (int i = 0; i < 4096; ++i) {
        int value = (bitsPerBlock == 15) ? blocks[i] : palette[blocks[i]];
        int bit_index = i * bitsPerBlock;
        int long_index = bit_index / 64;
        int long_offset = bit_index % 64;

        data[long_index] |= (uint64_t)value << long_offset;

        if (long_offset + bitsPerBlock > 64) {
            data[long_index + 1] |= (uint64_t)value >> (64 - long_offset);
        }
    }

    for (int i = 0; i < longs_size; ++i) {
        buffer.writeULong(data[i]);
    }
}

int ChunkColumn::getBlock(int x, int y, int z) const {
    if (y < 0 || y >= 256) return 0;
    int sectionIndex = y / 16;
    if (!sections[sectionIndex]) return 0;
    int blockIndex = (y % 16) * 16 * 16 + z * 16 + x;
    return sections[sectionIndex]->blocks[blockIndex];
}

void ChunkColumn::setBlock(int x, int y, int z, int blockId) {
    if (y < 0 || y >= 256) return;
    int sectionIndex = y / 16;
    if (!sections[sectionIndex]) {
        sections[sectionIndex] = std::make_unique<ChunkSection>();
    }
    int blockIndex = (y % 16) * 16 * 16 + z * 16 + x;
    sections[sectionIndex]->blocks[blockIndex] = blockId;
}

std::vector<uint8_t> ChunkColumn::serializeForFile() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(16 * 4096);
    for (const auto& section : sections) {
        if (section) {
            for (int blockId : section->blocks) {
                buffer.push_back(static_cast<uint8_t>(blockId));
            }
        } else {
            buffer.insert(buffer.end(), 4096, 0);
        }
    }
    return buffer;
}

void ChunkColumn::deserialize(const std::vector<uint8_t>& data) {
    int index = 0;
    for (auto& section : sections) {
        if (!section) {
            section = std::make_unique<ChunkSection>();
        }
        for (int& blockId : section->blocks) {
            if (index < data.size()) {
                blockId = data[index++];
            } else {
                blockId = 0;
            }
        }
    }
}

ChunkSection* ChunkColumn::getSection(int index) const {
    if (index < 0 || index >= 16) return nullptr;
    return sections[index].get();
}
