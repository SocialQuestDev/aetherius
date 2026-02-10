#pragma once

#include <map>
#include <memory>
#include <vector>
#include <cstdint>
#include "Chunk.h"
#include "WorldGenerator.h"
#include "../../other/Vector3.h"

class World {
public:
    World(std::unique_ptr<WorldGenerator> generator);

    ChunkColumn* getChunk(int x, int z);
    ChunkColumn* generateChunk(int x, int z);
    std::vector<uint8_t> getDimensionCodec();
    std::vector<uint8_t> getDimension();

    int getBlock(const Vector3& position);
    void setBlock(const Vector3& position, int blockId);

private:
    std::map<std::pair<int, int>, ChunkColumn> chunks;
    std::unique_ptr<WorldGenerator> generator;
};