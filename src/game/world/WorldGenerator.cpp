#include "../../../include/game/world/WorldGenerator.h"

void FlatWorldGenerator::generateChunk(ChunkColumn& chunkColumn) {
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            chunkColumn.setBlock(x, 0, z, 1); // Bedrock
            chunkColumn.setBlock(x, 1, z, 2); // Stone
            chunkColumn.setBlock(x, 2, z, 2); // Stone
            chunkColumn.setBlock(x, 3, z, 3); // Dirt
            chunkColumn.setBlock(x, 4, z, 4); // Grass
        }
    }
}
