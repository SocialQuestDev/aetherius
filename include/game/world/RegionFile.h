#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <array>
#include <memory>

const int REGION_WIDTH = 32;
const int REGION_SIZE = REGION_WIDTH * REGION_WIDTH;

class RegionFile {
public:
    RegionFile(const std::string& filePath);
    ~RegionFile();

    bool getChunkData(int chunkX, int chunkZ, std::vector<uint8_t>& data);
    void saveChunkData(int chunkX, int chunkZ, const std::vector<uint8_t>& data);

private:
    struct ChunkLocation {
        uint32_t offset;
        uint8_t size;
    };

    void readHeader();
    void writeHeader();
    int getChunkIndex(int chunkX, int chunkZ);

    std::string filePath_;
    std::fstream file_;
    std::mutex mutex_;
    std::array<ChunkLocation, REGION_SIZE> locationTable_{};
};
