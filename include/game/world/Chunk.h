#pragma once
#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <map>
#include <cmath>
#include <chrono>

#include "game/nbt/NbtBuilder.h"
#include "network/PacketBuffer.h"

// Defines whether a chunk is fully processed or just visual
enum class ChunkStatus {
    VISUAL,
    TICKING
};

class ChunkSection {
public:
    std::array<int, 4096> blocks{};

    ChunkSection() {
        blocks.fill(0);
    }

    std::vector<uint8_t> serialize() {
        PacketBuffer buf;
        short blockCount = 0;
        std::map<int, int> palette;

        for (int blockId : blocks) {
            if (blockId != 0) {
                blockCount++;
            }
            palette.emplace(blockId, 0);
        }
        buf.writeShort(blockCount);

        uint8_t bitsPerBlock = std::max(4, (int)ceil(log2(palette.size())));
        if (bitsPerBlock > 8) {
            bitsPerBlock = 15;
        }
        buf.writeByte(bitsPerBlock);

        if (bitsPerBlock != 15) {
            buf.writeVarInt(palette.size());
            int paletteIndex = 0;
            for (auto const& [blockId, val] : palette) {
                palette[blockId] = paletteIndex++;
                buf.writeVarInt(blockId);
            }
        }

        int longs_size = (4096 * bitsPerBlock) / 64;
        if ((4096 * bitsPerBlock) % 64 != 0) {
            longs_size++;
        }
        buf.writeVarInt(longs_size);
        
        std::vector<uint64_t> data(longs_size, 0);
        int bit_index = 0;
        for (int block : blocks) {
            int value = (bitsPerBlock == 15) ? block : palette[block];
            int long_index = bit_index / 64;
            int long_offset = bit_index % 64;

            data[long_index] |= (uint64_t)value << long_offset;

            int remaining_bits = 64 - long_offset;
            if (remaining_bits < bitsPerBlock) {
                data[long_index + 1] |= (uint64_t)value >> remaining_bits;
            }
            bit_index += bitsPerBlock;
        }

        for (uint64_t val : data) {
            buf.writeULong(val);
        }

        return buf.data;
    }
};

class ChunkColumn {
public:
    int x, z;
    std::array<ChunkSection, 16> sections{};

    std::chrono::steady_clock::time_point last_accessed;
    ChunkStatus status = ChunkStatus::VISUAL; // Default to visual-only

    ChunkColumn() : last_accessed(std::chrono::steady_clock::now()) {}

    void touch() {
        last_accessed = std::chrono::steady_clock::now();
    }

    int getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, int blockId);

    std::vector<uint8_t> serialize();
    std::vector<uint8_t> serializeForFile() const;
    void deserialize(const std::vector<uint8_t>& data);
};
