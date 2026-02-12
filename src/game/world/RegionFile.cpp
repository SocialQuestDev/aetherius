#include "game/world/RegionFile.h"
#include <iostream>

const int SECTOR_SIZE = 4096;
const int HEADER_SIZE = SECTOR_SIZE;

RegionFile::RegionFile(const std::string& filePath) : filePath_(filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.open(filePath_, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        file_.open(filePath_, std::ios::out | std::ios::binary);
        file_.close();
        file_.open(filePath_, std::ios::in | std::ios::out | std::ios::binary);

        std::vector<char> emptyHeader(HEADER_SIZE, 0);
        file_.write(emptyHeader.data(), emptyHeader.size());
        writeHeader();
    } else {
        readHeader();
    }
}

RegionFile::~RegionFile() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
}

int RegionFile::getChunkIndex(int chunkX, int chunkZ) {
    return (chunkX & (REGION_WIDTH - 1)) + (chunkZ & (REGION_WIDTH - 1)) * REGION_WIDTH;
}

void RegionFile::readHeader() {
    file_.seekg(0);
    for (int i = 0; i < REGION_SIZE; ++i) {
        uint32_t val;
        file_.read(reinterpret_cast<char*>(&val), sizeof(uint32_t));
        locationTable_[i].offset = (val >> 8);
        locationTable_[i].size = (val & 0xFF);
    }
}

void RegionFile::writeHeader() {
    file_.seekp(0);
    for (int i = 0; i < REGION_SIZE; ++i) {
        uint32_t val = (locationTable_[i].offset << 8) | locationTable_[i].size;
        file_.write(reinterpret_cast<const char*>(&val), sizeof(uint32_t));
    }
    file_.flush();
}

bool RegionFile::getChunkData(int chunkX, int chunkZ, std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    int index = getChunkIndex(chunkX, chunkZ);
    const auto& loc = locationTable_[index];

    if (loc.offset == 0 || loc.size == 0) {
        return false;
    }

    file_.seekg(loc.offset * SECTOR_SIZE);
    uint32_t dataSize;
    file_.read(reinterpret_cast<char*>(&dataSize), sizeof(uint32_t));

    data.resize(dataSize);
    file_.read(reinterpret_cast<char*>(data.data()), dataSize);

    return true;
}

void RegionFile::saveChunkData(int chunkX, int chunkZ, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    int index = getChunkIndex(chunkX, chunkZ);

    uint32_t dataSize = data.size();
    uint8_t sectorsNeeded = (dataSize + sizeof(uint32_t) + SECTOR_SIZE - 1) / SECTOR_SIZE;

    auto& loc = locationTable_[index];

    if (loc.offset != 0 && loc.size >= sectorsNeeded) {
        file_.seekp(loc.offset * SECTOR_SIZE);
    } else {
        file_.seekp(0, std::ios::end);
        uint32_t newOffset = (file_.tellp() + SECTOR_SIZE - 1) / SECTOR_SIZE;
        if (newOffset == 0) newOffset = 1;
        loc.offset = newOffset;
        loc.size = sectorsNeeded;
        writeHeader();
        file_.seekp(loc.offset * SECTOR_SIZE);
    }

    file_.write(reinterpret_cast<const char*>(&dataSize), sizeof(uint32_t));
    file_.write(reinterpret_cast<const char*>(data.data()), dataSize);
    file_.flush();
}
