#pragma once

#include "game/world/Chunk.h"

class WorldGenerator {
public:
    virtual ~WorldGenerator() = default;
    virtual void generateChunk(ChunkColumn& chunk) = 0;
};

class FlatWorldGenerator : public WorldGenerator {
public:
    void generateChunk(ChunkColumn& chunk) override;
};
