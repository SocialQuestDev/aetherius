#pragma once

#include "game/world/Chunk.h"
#include "game/world/RegionFile.h"
#include <string>
#include <memory>
#include <map>
#include <mutex>

class WorldStorage {
public:
    WorldStorage(const std::string& worldName);
    bool loadChunk(int x, int z, ChunkColumn& chunk);
    void saveChunkData(int x, int z, const std::vector<uint8_t>& data);

private:
    RegionFile* getRegionFile(int regionX, int regionZ);

    std::string worldPath_;
    std::map<std::pair<int, int>, std::unique_ptr<RegionFile>> regionFiles_;
    std::mutex regionFilesMutex_;
};
