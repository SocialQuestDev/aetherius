#include "game/world/WorldStorage.h"
#include <filesystem>

namespace fs = std::filesystem;

WorldStorage::WorldStorage(const std::string& worldName) {
    worldPath_ = "worlds/" + worldName + "/region";
    fs::create_directories(worldPath_);
}

RegionFile* WorldStorage::getRegionFile(int regionX, int regionZ) {
    std::lock_guard<std::mutex> lock(regionFilesMutex_);
    auto it = regionFiles_.find({regionX, regionZ});
    if (it != regionFiles_.end()) {
        return it->second.get();
    }

    std::string filePath = worldPath_ + "/r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".dat";
    auto regionFile = std::make_unique<RegionFile>(filePath);
    auto* ptr = regionFile.get();
    regionFiles_.emplace(std::make_pair(regionX, regionZ), std::move(regionFile));
    return ptr;
}

bool WorldStorage::loadChunk(int x, int z, ChunkColumn& chunk) {
    int regionX = x >> 5; // x / 32
    int regionZ = z >> 5; // z / 32
    RegionFile* region = getRegionFile(regionX, regionZ);

    std::vector<uint8_t> data;
    if (region->getChunkData(x, z, data)) {
        chunk.deserialize(data);
        return true;
    }
    return false;
}

void WorldStorage::saveChunk(const ChunkColumn& chunk) {
    int regionX = chunk.x >> 5;
    int regionZ = chunk.z >> 5;
    RegionFile* region = getRegionFile(regionX, regionZ);

    std::vector<uint8_t> data = chunk.serializeForFile();
    region->saveChunkData(chunk.x, chunk.z, data);
}
