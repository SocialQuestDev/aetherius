#include "game/world/Chunk.h"
#include <numeric>
#include <unordered_map>
#include <algorithm>

namespace {
constexpr uint8_t kFileMagic[4] = {'A', 'E', 'T', 'H'};
constexpr uint8_t kFileVersion = 1;
}

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
    // Fast path: count non-air blocks and build palette
    short blockCount = 0;
    std::vector<int> unique_blocks;
    unique_blocks.reserve(16); // Most chunks have < 16 unique block types

    // Use unordered_map for faster lookups during palette building
    std::unordered_map<int, int> palette_map;
    palette_map.reserve(16);
    palette_map[0] = 0;
    unique_blocks.push_back(0);

    for (int blockId : blocks) {
        if (blockId != 0) {
            blockCount++;
            if (palette_map.find(blockId) == palette_map.end()) {
                palette_map[blockId] = unique_blocks.size();
                unique_blocks.push_back(blockId);
            }
        }
    }

    buffer.writeShort(blockCount);

    // Calculate bits per block
    uint8_t bitsPerBlock = std::max(4, (int)ceil(log2(unique_blocks.size())));
    if (bitsPerBlock > 8) {
        bitsPerBlock = 15;
    }
    buffer.writeByte(bitsPerBlock);

    // Write palette (if not direct)
    if (bitsPerBlock != 15) {
        buffer.writeVarInt(unique_blocks.size());
        for (int blockId : unique_blocks) {
            buffer.writeVarInt(blockId);
        }
    }

    // Optimized block data packing
    const int longs_size = (4096 * bitsPerBlock + 63) / 64;
    buffer.writeVarInt(longs_size);

    std::array<uint64_t, 512> data{};

    // Fast packing loop with minimal branches
    if (bitsPerBlock == 15) {
        // Direct mode - no palette lookup needed
        for (int i = 0; i < 4096; ++i) {
            int bit_index = i * 15;
            int long_index = bit_index / 64;
            int long_offset = bit_index % 64;

            data[long_index] |= (uint64_t)blocks[i] << long_offset;

            if (long_offset + 15 > 64) {
                data[long_index + 1] |= (uint64_t)blocks[i] >> (64 - long_offset);
            }
        }
    } else {
        // Palette mode
        for (int i = 0; i < 4096; ++i) {
            int value = palette_map[blocks[i]];
            int bit_index = i * bitsPerBlock;
            int long_index = bit_index / 64;
            int long_offset = bit_index % 64;

            data[long_index] |= (uint64_t)value << long_offset;

            if (long_offset + bitsPerBlock > 64) {
                data[long_index + 1] |= (uint64_t)value >> (64 - long_offset);
            }
        }
    }

    // Batch write longs
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
    invalidateNetworkCache();
}

std::vector<uint8_t> ChunkColumn::serializeForFile() const {
    bool all_empty = true;
    for (const auto& section : sections) {
        if (section && !section->isEmpty()) {
            all_empty = false;
            break;
        }
    }

    if (all_empty) {
        return {};
    }

    PacketBuffer buffer;
    buffer.writeByte(kFileMagic[0]);
    buffer.writeByte(kFileMagic[1]);
    buffer.writeByte(kFileMagic[2]);
    buffer.writeByte(kFileMagic[3]);
    buffer.writeByte(kFileVersion);

    for (const auto& section : sections) {
        if (!section || section->isEmpty()) {
            buffer.writeByte(0);
            continue;
        }

        buffer.writeByte(1);
        const auto& blocks = section->blocks;
        int idx = 0;
        while (idx < 4096) {
            const int blockId = blocks[idx];
            int run_len = 1;
            while (idx + run_len < 4096 && blocks[idx + run_len] == blockId) {
                ++run_len;
            }
            buffer.writeVarInt(blockId);
            buffer.writeVarInt(run_len);
            idx += run_len;
        }
    }

    return buffer.data;
}

void ChunkColumn::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() >= 5 &&
        data[0] == kFileMagic[0] &&
        data[1] == kFileMagic[1] &&
        data[2] == kFileMagic[2] &&
        data[3] == kFileMagic[3] &&
        data[4] == kFileVersion) {

        std::vector<uint8_t> temp = data;
        PacketBuffer buffer(temp);
        buffer.readByte(); // A
        buffer.readByte(); // E
        buffer.readByte(); // T
        buffer.readByte(); // H
        buffer.readByte(); // version

        for (auto& section : sections) {
            if (!section) {
                section = std::make_unique<ChunkSection>();
            }

            if (buffer.readerIndex >= buffer.data.size()) {
                section->blocks.fill(0);
                continue;
            }

            uint8_t marker = buffer.readByte();
            if (marker == 0) {
                section->blocks.fill(0);
                continue;
            }

            int filled = 0;
            while (filled < 4096 && buffer.readerIndex < buffer.data.size()) {
                int blockId = buffer.readVarInt();
                int run_len = buffer.readVarInt();
                if (run_len <= 0) {
                    break;
                }
                int end = std::min(4096, filled + run_len);
                for (int i = filled; i < end; ++i) {
                    section->blocks[i] = blockId;
                }
                filled = end;
            }

            if (filled < 4096) {
                for (int i = filled; i < 4096; ++i) {
                    section->blocks[i] = 0;
                }
            }
        }

        return;
    }

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

std::vector<uint8_t> ChunkColumn::getCachedNetworkData() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (network_cache_valid_) {
        return network_cache_;
    }

    // Serialize chunk data for network
    PacketBuffer buffer;

    // Calculate primary bitmask
    int primaryBitmask = 0;
    for (int i = 0; i < 16; ++i) {
        if (sections[i] != nullptr && !sections[i]->isEmpty()) {
            primaryBitmask |= (1 << i);
        }
    }

    // Serialize all sections
    for (int i = 0; i < 16; ++i) {
        if ((primaryBitmask >> i) & 1) {
            sections[i]->writeToNetwork(buffer);
        }
    }

    network_cache_ = std::move(buffer.data);
    network_cache_valid_ = true;

    return network_cache_;
}

void ChunkColumn::invalidateNetworkCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    network_cache_valid_ = false;
}
