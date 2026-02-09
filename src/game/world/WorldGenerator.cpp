#include "../../../include/game/world/WorldGenerator.h"

void FlatWorldGenerator::generateChunk(ChunkColumn& chunkColumn) {
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            chunkColumn.setBlock(x, 0, z, 35); // Bedrock

            for (int y = 1; y < 66; ++y) {
                chunkColumn.setBlock(x, y, z, 1); // Stone
            }

            for (int y = 66; y < 69; ++y) {
                chunkColumn.setBlock(x, y, z, 10); // Dirt
            }

            chunkColumn.setBlock(x, 69, z, 9); // Grass
        }
    }
}
