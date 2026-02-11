#pragma once

#include <map>
#include <memory>
#include <vector>
#include <cstdint>
#include <mutex>
#include <future>
#include <boost/asio/thread_pool.hpp>
#include "game/world/Chunk.h"
#include "game/world/WorldGenerator.h"
#include "other/Vector3.h"

class World {
public:
    World(std::unique_ptr<WorldGenerator> generator);

    ChunkColumn* getChunk(int x, int z);
    ChunkColumn* generateChunk(int x, int z);
    std::future<ChunkColumn*> generateChunkAsync(int x, int z);
    std::vector<uint8_t> getDimensionCodec();
    std::vector<uint8_t> getDimension();

    int getBlock(const Vector3& position);
    void setBlock(const Vector3& position, int blockId);

    // Time system (20 ticks per second, 24000 ticks per day)
    void tick();
    int64_t getWorldAge() const;
    int64_t getTimeOfDay() const;
    void setTimeOfDay(int64_t time);

private:
    std::map<std::pair<int, int>, ChunkColumn> chunks;
    std::unique_ptr<WorldGenerator> generator;
    std::mutex chunks_mutex_;
    std::unique_ptr<boost::asio::thread_pool> thread_pool_;

    // Time tracking
    int64_t worldAge = 0;      // Total ticks since world creation
    int64_t timeOfDay = 0;     // Current time (0-24000: 0=sunrise, 6000=noon, 12000=sunset, 18000=midnight)
};