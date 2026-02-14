#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include "game/world/Chunk.h"

class WorldGenerator;
class WorldStorage;

struct ChunkCoord {
    int x, z;

    ChunkCoord(int x_, int z_) : x(x_), z(z_) {}

    bool operator==(const ChunkCoord& other) const {
        return x == other.x && z == other.z;
    }

    struct Hash {
        size_t operator()(const ChunkCoord& coord) const {
            return std::hash<int>()(coord.x) ^ (std::hash<int>()(coord.z) << 1);
        }
    };
};

struct ChunkLoadTask {
    ChunkCoord coord;
    int priority;  // Lower = higher priority (distance from origin)
    std::function<void(ChunkColumn*)> callback;

    ChunkLoadTask(ChunkCoord c, int p, std::function<void(ChunkColumn*)> cb)
        : coord(c), priority(p), callback(std::move(cb)) {}

    // For priority queue (inverted: higher priority = lower distance)
    bool operator<(const ChunkLoadTask& other) const {
        return priority > other.priority;  // Min-heap based on priority
    }
};

struct CompletedChunkTask {
    std::unique_ptr<ChunkColumn> chunk;
    std::function<void(ChunkColumn*)> callback;

    CompletedChunkTask(std::unique_ptr<ChunkColumn> c, std::function<void(ChunkColumn*)> cb)
        : chunk(std::move(c)), callback(std::move(cb)) {}
};

class ChunkManager {
public:
    ChunkManager(WorldGenerator* generator, WorldStorage* storage, size_t num_threads = 0);
    ~ChunkManager();

    // Non-copyable, non-movable
    ChunkManager(const ChunkManager&) = delete;
    ChunkManager& operator=(const ChunkManager&) = delete;

    /**
     * Get a chunk synchronously. Returns nullptr if not loaded.
     * Does NOT trigger loading - call requestChunk() for that.
     */
    ChunkColumn* getChunk(int x, int z);

    /**
     * Request a chunk to be loaded/generated asynchronously.
     * If already loaded, callback is invoked immediately.
     * If pending, adds callback to existing request.
     * Otherwise, queues a new load task.
     *
     * @param x Chunk X coordinate
     * @param z Chunk Z coordinate
     * @param priority Distance-based priority (lower = more important)
     * @param callback Called when chunk is ready (may be on worker thread or main thread)
     */
    void requestChunk(int x, int z, int priority, std::function<void(ChunkColumn*)> callback);

    /**
     * Process completed chunks on the main thread.
     * Call this from your server tick loop.
     * Returns number of chunks processed.
     */
    int processCompletedChunks();

    /**
     * Manually insert a chunk (for pre-loaded chunks)
     */
    void insertChunk(std::unique_ptr<ChunkColumn> chunk);

    /**
     * Remove chunk from memory (for unloading)
     * Returns the chunk if it exists, nullptr otherwise
     */
    std::unique_ptr<ChunkColumn> removeChunk(int x, int z);

    /**
     * Check if a chunk is loaded
     */
    bool isChunkLoaded(int x, int z) const;

    /**
     * Check if a chunk is currently being loaded
     */
    bool isChunkPending(int x, int z) const;

    /**
     * Get all loaded chunks (thread-safe)
     */
    std::vector<ChunkColumn*> getAllChunks() const;

    /**
     * Calculate spiral priority based on distance from origin
     */
    static int calculatePriority(int x, int z, int originX = 0, int originZ = 0);

    /**
     * Shutdown the chunk manager and join all threads
     */
    void shutdown();

private:
    void workerThread();
    void loadOrGenerateChunk(const ChunkCoord& coord, std::function<void(ChunkColumn*)> callback);

    // Chunk storage
    std::unordered_map<ChunkCoord, std::unique_ptr<ChunkColumn>, ChunkCoord::Hash> loaded_chunks_;
    mutable std::mutex chunks_mutex_;

    // Task queue
    std::priority_queue<ChunkLoadTask> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // Pending chunks (being loaded)
    std::unordered_map<ChunkCoord, std::vector<std::function<void(ChunkColumn*)>>, ChunkCoord::Hash> pending_chunks_;
    mutable std::mutex pending_mutex_;

    // Completed chunks (ready for main thread processing)
    std::vector<CompletedChunkTask> completed_chunks_;
    std::mutex completed_mutex_;

    // Worker threads
    std::vector<std::thread> workers_;
    std::atomic<bool> shutdown_flag_;

    // Generation and storage
    WorldGenerator* generator_;
    WorldStorage* storage_;
};
