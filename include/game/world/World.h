#pragma once

#include <map>
#include <memory>
#include <vector>
#include <cstdint>
#include <mutex>
#include <functional>
#include <unordered_set>
#include "game/world/Chunk.h"
#include "game/world/WorldGenerator.h"
#include "game/world/WorldStorage.h"
#include "game/world/ChunkManager.h"
#include "other/Vector3.h"

class World {
public:
    World(std::unique_ptr<WorldGenerator> generator, const std::string& worldName);

    ChunkColumn* getChunk(int x, int z);
    void requestChunk(int x, int z, int priority, std::function<void(ChunkColumn*)> callback);
    void getOrGenerateChunk(int x, int z, std::function<void(ChunkColumn*)> callback);  // Deprecated but kept for compatibility
    void flushCompletedChunks();  // Force process all completed chunks immediately

    static std::vector<uint8_t> getDimensionCodec();
    static std::vector<uint8_t> getDimension();

    int getBlock(const Vector3& position);
    void setBlock(const Vector3& position, int blockId);

    void syncSaveAllChunks();
    void syncSaveDirtyChunks();
    void asyncSaveChunk(const ChunkColumn& chunk);
    void unloadInactiveChunks();
    void updateChunkStatuses();

    void tick();
    int64_t getWorldAge() const;
    int64_t getTimeOfDay() const;
    void setTimeOfDay(int64_t time);

    ChunkManager& getChunkManager() { return *chunk_manager_; }

private:
    struct ChunkCoordHash {
        std::size_t operator()(const std::pair<int, int>& coord) const noexcept {
            return (static_cast<std::size_t>(coord.first) << 32) ^
                   static_cast<std::size_t>(coord.second);
        }
    };

    void markChunkDirty(int x, int z);

    std::unique_ptr<WorldGenerator> generator;
    std::unique_ptr<WorldStorage> storage;
    std::unique_ptr<ChunkManager> chunk_manager_;
    std::unordered_set<std::pair<int, int>, ChunkCoordHash> dirty_chunks_;
    std::mutex dirty_mutex_;

    int64_t worldAge = 0;
    int64_t timeOfDay = 0;
};
