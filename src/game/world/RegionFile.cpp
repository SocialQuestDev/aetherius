#include "game/world/RegionFile.h"
#include <iostream>
#include <zlib.h>

namespace {
constexpr int kChunkDataSize = 16 * 4096;
constexpr uint8_t kCompressionNone = 0;
constexpr uint8_t kCompressionZlib = 2;
}

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

    if (dataSize == 0) {
        return false;
    }

    std::vector<uint8_t> raw;
    raw.resize(dataSize);
    file_.read(reinterpret_cast<char*>(raw.data()), dataSize);

    if (dataSize == kChunkDataSize) {
        // Legacy: raw data without compression byte
        data = std::move(raw);
        return true;
    }

    if (!raw.empty() && (raw[0] == kCompressionZlib || raw[0] == kCompressionNone)) {
        const uint8_t compressionType = raw[0];
        if (compressionType == kCompressionZlib) {
            data.resize(kChunkDataSize);
            uLongf destLen = data.size();
            if (uncompress(data.data(), &destLen, raw.data() + 1, raw.size() - 1) != Z_OK ||
                destLen != data.size()) {
                return false;
            }
            return true;
        }

        if (compressionType == kCompressionNone) {
            data.assign(raw.begin() + 1, raw.end());
            return true;
        }
    }

    return false;
}

void RegionFile::saveChunkData(int chunkX, int chunkZ, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    int index = getChunkIndex(chunkX, chunkZ);

    std::vector<uint8_t> compressed;
    compressed.resize(compressBound(data.size()));
    uLongf compressedSize = compressed.size();
    bool compressed_ok = (compress2(compressed.data(), &compressedSize, data.data(), data.size(), Z_BEST_COMPRESSION) == Z_OK);
    if (compressed_ok) {
        compressed.resize(compressedSize);
    } else {
        compressed.clear();
    }

    std::vector<uint8_t> payload;
    if (compressed_ok && compressed.size() < data.size()) {
        payload.reserve(compressed.size() + 1);
        payload.push_back(kCompressionZlib);
        payload.insert(payload.end(), compressed.begin(), compressed.end());
    } else {
        payload.reserve(data.size() + 1);
        payload.push_back(kCompressionNone);
        payload.insert(payload.end(), data.begin(), data.end());
    }

    uint32_t dataSize = payload.size();
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
    file_.write(reinterpret_cast<const char*>(payload.data()), dataSize);
    file_.flush();
}

void RegionFile::deleteChunkData(int chunkX, int chunkZ) {
    std::lock_guard<std::mutex> lock(mutex_);
    int index = getChunkIndex(chunkX, chunkZ);

    auto& loc = locationTable_[index];
    if (loc.offset == 0 && loc.size == 0) {
        return;
    }

    loc.offset = 0;
    loc.size = 0;
    writeHeader();
}
