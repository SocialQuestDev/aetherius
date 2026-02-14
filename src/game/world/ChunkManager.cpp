#include "game/world/ChunkManager.h"
#include "game/world/WorldGenerator.h"
#include "game/world/WorldStorage.h"
#include "console/Logger.h"
#include <algorithm>
#include <cmath>

ChunkManager::ChunkManager(WorldGenerator* generator, WorldStorage* storage, size_t num_threads)
    : generator_(generator),
      storage_(storage),
      shutdown_flag_(false) {

    if (!generator_) {
        LOG_ERROR("ChunkManager: generator is null!");
    }
    if (!storage_) {
        LOG_ERROR("ChunkManager: storage is null!");
    }

    // Default to hardware concurrency if not specified
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;  // Fallback
    }

    // Spawn worker threads
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ChunkManager::workerThread, this);
    }

    LOG_DEBUG("ChunkManager initialized with " + std::to_string(num_threads) + " worker threads");
}

ChunkManager::~ChunkManager() {
    shutdown();
}

void ChunkManager::shutdown() {
    if (shutdown_flag_.exchange(true)) {
        return;  // Already shut down
    }

    // Wake up all workers
    queue_cv_.notify_all();

    // Join all threads
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
    LOG_DEBUG("ChunkManager shut down");
}

ChunkColumn* ChunkManager::getChunk(int x, int z) {
    std::lock_guard<std::mutex> lock(chunks_mutex_);
    auto it = loaded_chunks_.find(ChunkCoord(x, z));
    if (it != loaded_chunks_.end()) {
        it->second->touch();
        return it->second.get();
    }
    return nullptr;
}

void ChunkManager::requestChunk(int x, int z, int priority, std::function<void(ChunkColumn*)> callback) {
    ChunkCoord coord(x, z);

    // Check if already loaded
    {
        std::lock_guard<std::mutex> lock(chunks_mutex_);
        auto it = loaded_chunks_.find(coord);
        if (it != loaded_chunks_.end()) {
            it->second->touch();
            callback(it->second.get());
            return;
        }
    }

    // Check if already pending
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        auto it = pending_chunks_.find(coord);
        if (it != pending_chunks_.end()) {
            // Add callback to existing request
            it->second.push_back(std::move(callback));
            return;
        }
        // Mark as pending
        pending_chunks_[coord].push_back(std::move(callback));
    }

    // Queue the load task
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.emplace(coord, priority, nullptr);  // Callbacks stored in pending_chunks_
    }
    queue_cv_.notify_one();
}

int ChunkManager::processCompletedChunks() {
    std::vector<CompletedChunkTask> to_process;

    // Move completed chunks to local vector
    {
        std::lock_guard<std::mutex> lock(completed_mutex_);
        if (completed_chunks_.empty()) {
            return 0;
        }
        to_process = std::move(completed_chunks_);
        completed_chunks_.clear();
    }

    int processed = 0;
    for (auto& task : to_process) {
        if (task.chunk) {
            ChunkCoord coord(task.chunk->getX(), task.chunk->getZ());

            // Insert into loaded chunks
            {
                std::lock_guard<std::mutex> lock(chunks_mutex_);
                task.chunk->touch();
                loaded_chunks_[coord] = std::move(task.chunk);
            }

            // Invoke callback on main thread
            if (task.callback) {
                task.callback(loaded_chunks_[coord].get());
            }

            processed++;
        }
    }

    return processed;
}

void ChunkManager::insertChunk(std::unique_ptr<ChunkColumn> chunk) {
    if (!chunk) return;

    ChunkCoord coord(chunk->getX(), chunk->getZ());
    std::lock_guard<std::mutex> lock(chunks_mutex_);
    chunk->touch();
    loaded_chunks_[coord] = std::move(chunk);
}

std::unique_ptr<ChunkColumn> ChunkManager::removeChunk(int x, int z) {
    ChunkCoord coord(x, z);
    std::lock_guard<std::mutex> lock(chunks_mutex_);

    auto it = loaded_chunks_.find(coord);
    if (it != loaded_chunks_.end()) {
        auto chunk = std::move(it->second);
        loaded_chunks_.erase(it);
        return chunk;
    }
    return nullptr;
}

bool ChunkManager::isChunkLoaded(int x, int z) const {
    std::lock_guard<std::mutex> lock(chunks_mutex_);
    return loaded_chunks_.find(ChunkCoord(x, z)) != loaded_chunks_.end();
}

bool ChunkManager::isChunkPending(int x, int z) const {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    return pending_chunks_.find(ChunkCoord(x, z)) != pending_chunks_.end();
}

std::vector<ChunkColumn*> ChunkManager::getAllChunks() const {
    std::lock_guard<std::mutex> lock(chunks_mutex_);
    std::vector<ChunkColumn*> result;
    result.reserve(loaded_chunks_.size());
    for (const auto& pair : loaded_chunks_) {
        result.push_back(pair.second.get());
    }
    return result;
}

int ChunkManager::calculatePriority(int x, int z, int originX, int originZ) {
    // Manhattan distance for spiral loading
    int dx = std::abs(x - originX);
    int dz = std::abs(z - originZ);
    return dx + dz;
}

void ChunkManager::workerThread() {
    while (!shutdown_flag_.load()) {
        ChunkLoadTask task(ChunkCoord(0, 0), 0, nullptr);

        // Wait for a task
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] {
                return !task_queue_.empty() || shutdown_flag_.load();
            });

            if (shutdown_flag_.load() && task_queue_.empty()) {
                return;
            }

            if (!task_queue_.empty()) {
                task = task_queue_.top();
                task_queue_.pop();
            } else {
                continue;
            }
        }

        // Get callbacks for this chunk
        std::vector<std::function<void(ChunkColumn*)>> callbacks;
        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            auto it = pending_chunks_.find(task.coord);
            if (it != pending_chunks_.end()) {
                callbacks = std::move(it->second);
                pending_chunks_.erase(it);
            }
        }

        // Load or generate the chunk
        auto chunk = std::make_unique<ChunkColumn>(task.coord.x, task.coord.z);

        bool loaded_from_disk = false;
        if (storage_) {
            try {
                loaded_from_disk = storage_->loadChunk(task.coord.x, task.coord.z, *chunk);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to load chunk (" + std::to_string(task.coord.x) + ", " +
                         std::to_string(task.coord.z) + "): " + e.what());
            }
        }

        if (!loaded_from_disk && generator_) {
            try {
                generator_->generateChunk(*chunk);

                // Save newly generated chunk to disk
                if (storage_) {
                    auto dataToSave = chunk->serializeForFile();
                    storage_->saveChunkData(task.coord.x, task.coord.z, dataToSave);
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to generate chunk (" + std::to_string(task.coord.x) + ", " +
                         std::to_string(task.coord.z) + "): " + e.what());
            }
        }

        // Add to completed queue for main thread processing
        // Combine all callbacks into one
        if (!callbacks.empty()) {
            auto combined_callback = [callbacks = std::move(callbacks)](ChunkColumn* chunk_ptr) {
                for (const auto& cb : callbacks) {
                    if (cb) cb(chunk_ptr);
                }
            };

            std::lock_guard<std::mutex> lock(completed_mutex_);
            completed_chunks_.emplace_back(std::move(chunk), std::move(combined_callback));
        } else {
            // No callbacks, still add to completed queue
            std::lock_guard<std::mutex> lock(completed_mutex_);
            completed_chunks_.emplace_back(std::move(chunk), nullptr);
        }
    }
}

void ChunkManager::loadOrGenerateChunk(const ChunkCoord& coord, std::function<void(ChunkColumn*)> callback) {
    // This method is no longer needed as the logic is in workerThread()
    // Kept for potential future use
}
