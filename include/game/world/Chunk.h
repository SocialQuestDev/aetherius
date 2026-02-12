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

    bool isEmpty() const;
    void writeToNetwork(PacketBuffer& buffer);
};

class ChunkColumn {
public:
    int x, z;
    std::array<std::unique_ptr<ChunkSection>, 16> sections;

    std::chrono::steady_clock::time_point last_accessed;
    ChunkStatus status = ChunkStatus::VISUAL;

    ChunkColumn(int x, int z);

    void touch();

    int getX() const { return x; }
    int getZ() const { return z; }
    ChunkSection* getSection(int index) const;

    int getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, int blockId);

    std::vector<uint8_t> serializeForFile() const;
    void deserialize(const std::vector<uint8_t>& data);
};
