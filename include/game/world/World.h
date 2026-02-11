#pragma once

#include <map>
#include <memory>
#include <vector>
#include <cstdint>
#include <mutex>
#include <functional>
#include <boost/asio/thread_pool.hpp>
#include "game/world/Chunk.h"
#include "game/world/WorldGenerator.h"
#include "game/world/WorldStorage.h"
#include "other/Vector3.h"

class World {
public:
    World(std::unique_ptr<WorldGenerator> generator, const std::string& worldName);

    ChunkColumn* getChunk(int x, int z);
    void getOrGenerateChunk(int x, int z, std::function<void(ChunkColumn*)> callback);
    static std::vector<uint8_t> getDimensionCodec();
    static std::vector<uint8_t> getDimension();

    int getBlock(const Vector3& position);
    void setBlock(const Vector3& position, int blockId);

    void syncSaveAllChunks();
    void asyncSaveChunk(const ChunkColumn& chunk);
    void unloadInactiveChunks();
    void updateChunkStatuses();

    // Time system (20 ticks per second, 24000 ticks per day)
    void tick();
    int64_t getWorldAge() const;
    int64_t getTimeOfDay() const;
    void setTimeOfDay(int64_t time);

private:
    std::map<std::pair<int, int>, ChunkColumn> chunks;
    std::unique_ptr<WorldGenerator> generator;
    std::unique_ptr<WorldStorage> storage;
    std::mutex chunks_mutex_;
    std::mutex generation_mutex_;
    std::unique_ptr<boost::asio::thread_pool> thread_pool_;

    // Time tracking
    int64_t worldAge = 0;
    int64_t timeOfDay = 0;
};
