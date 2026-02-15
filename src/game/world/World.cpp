#include "game/world/World.h"
#include "game/nbt/NbtBuilder.h"
#include "game/player/PlayerList.h"
#include "console/Logger.h"
#include <cmath>
#include <vector>
#include <set>
#include <thread>

World::World(std::unique_ptr<WorldGenerator> generator_param, const std::string& worldName)
    : generator(std::move(generator_param)),
      storage(std::make_unique<WorldStorage>(worldName)),
      chunk_manager_(std::make_unique<ChunkManager>(generator.get(), storage.get())) {}

ChunkColumn* World::getChunk(int x, int z) {
    return chunk_manager_->getChunk(x, z);
}

void World::requestChunk(int x, int z, int priority, std::function<void(ChunkColumn*)> callback) {
    // Process any completed chunks before requesting new ones
    chunk_manager_->processCompletedChunks();
    chunk_manager_->requestChunk(x, z, priority, std::move(callback));
}

void World::getOrGenerateChunk(int x, int z, std::function<void(ChunkColumn*)> callback) {
    // Deprecated: Use requestChunk with priority instead
    // Default priority = 0 for backwards compatibility
    chunk_manager_->requestChunk(x, z, 0, std::move(callback));
}

void World::flushCompletedChunks() {
    chunk_manager_->processCompletedChunks();
}

void World::asyncSaveChunk(const ChunkColumn& chunk) {
    // Save is now handled by ChunkManager during generation
    // For runtime saves (e.g., block changes), mark dirty and defer to autosave
    markChunkDirty(chunk.getX(), chunk.getZ());
}

void World::syncSaveAllChunks() {
    auto chunks = chunk_manager_->getAllChunks();
    for (ChunkColumn* chunk : chunks) {
        if (chunk) {
            storage->saveChunkData(chunk->getX(), chunk->getZ(), chunk->serializeForFile());
        }
    }

    std::lock_guard<std::mutex> lock(dirty_mutex_);
    dirty_chunks_.clear();
}

void World::syncSaveDirtyChunks() {
    std::vector<std::pair<int, int>> dirty;
    {
        std::lock_guard<std::mutex> lock(dirty_mutex_);
        dirty.assign(dirty_chunks_.begin(), dirty_chunks_.end());
        dirty_chunks_.clear();
    }

    for (const auto& coord : dirty) {
        ChunkColumn* chunk = chunk_manager_->getChunk(coord.first, coord.second);
        if (chunk) {
            storage->saveChunkData(chunk->getX(), chunk->getZ(), chunk->serializeForFile());
        }
    }
}

void World::updateChunkStatuses() {
    std::set<std::pair<int, int>> active_chunks_coords;
    const int ticking_radius = 2;

    for (const auto& player : PlayerList::getInstance().getPlayers()) {
        Vector3 pos = player->getPosition();
        int chunkX = static_cast<int>(std::floor(pos.x / 16.0));
        int chunkZ = static_cast<int>(std::floor(pos.z / 16.0));

        for (int x = -ticking_radius; x <= ticking_radius; ++x) {
            for (int z = -ticking_radius; z <= ticking_radius; ++z) {
                active_chunks_coords.insert({chunkX + x, chunkZ + z});
            }
        }
    }

    auto chunks = chunk_manager_->getAllChunks();
    for (ChunkColumn* chunk : chunks) {
        if (chunk) {
            std::pair<int, int> pos = {chunk->getX(), chunk->getZ()};
            if (active_chunks_coords.contains(pos)) {
                chunk->status = ChunkStatus::TICKING;
            } else {
                chunk->status = ChunkStatus::VISUAL;
            }
        }
    }
}

void World::unloadInactiveChunks() {
    const auto unload_threshold = std::chrono::seconds(30);
    std::vector<std::pair<int, int>> to_unload_coords;

    std::set<std::pair<int, int>> keep_alive_chunks;
    for (const auto& player : PlayerList::getInstance().getPlayers()) {
        Vector3 pos = player->getPosition();
        int chunkX = static_cast<int>(std::floor(pos.x / 16.0));
        int chunkZ = static_cast<int>(std::floor(pos.z / 16.0));
        int view_dist = player->getViewDistance() + 1;
        for (int x = -view_dist; x <= view_dist; ++x) {
            for (int z = -view_dist; z <= view_dist; ++z) {
                keep_alive_chunks.insert({chunkX + x, chunkZ + z});
            }
        }
    }

    auto chunks = chunk_manager_->getAllChunks();
    for (ChunkColumn* chunk : chunks) {
        if (chunk) {
            std::pair<int, int> pos = {chunk->getX(), chunk->getZ()};
            if (!keep_alive_chunks.contains(pos)) {
                if (std::chrono::steady_clock::now() - chunk->last_accessed > unload_threshold) {
                    to_unload_coords.push_back(pos);
                }
            }
        }
    }

    if (!to_unload_coords.empty()) {
        for (const auto& pos : to_unload_coords) {
            auto chunk_to_save = chunk_manager_->removeChunk(pos.first, pos.second);
            if (chunk_to_save) {
                bool should_save = false;
                {
                    std::lock_guard<std::mutex> lock(dirty_mutex_);
                    auto it = dirty_chunks_.find(pos);
                    if (it != dirty_chunks_.end()) {
                        dirty_chunks_.erase(it);
                        should_save = true;
                    }
                }
                if (should_save) {
                    storage->saveChunkData(chunk_to_save->getX(), chunk_to_save->getZ(),
                                          chunk_to_save->serializeForFile());
                }
            }
        }
        LOG_DEBUG("Unloaded " + std::to_string(to_unload_coords.size()) + " inactive chunks.");
    }
}

void World::markChunkDirty(int x, int z) {
    std::lock_guard<std::mutex> lock(dirty_mutex_);
    dirty_chunks_.emplace(x, z);
}

int World::getBlock(const Vector3& position) {
    int chunkX = static_cast<int>(floor(position.x / 16.0));
    int chunkZ = static_cast<int>(floor(position.z / 16.0));

    ChunkColumn* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) return 0;

    int blockX = static_cast<int>(floor(position.x)) % 16;
    int blockY = static_cast<int>(floor(position.y));
    int blockZ = static_cast<int>(floor(position.z)) % 16;
    if (blockX < 0) blockX += 16;
    if (blockZ < 0) blockZ += 16;

    return chunk->getBlock(blockX, blockY, blockZ);
}

void World::setBlock(const Vector3& position, int blockId) {
    int chunkX = static_cast<int>(floor(position.x / 16.0));
    int chunkZ = static_cast<int>(floor(position.z / 16.0));

    ChunkColumn* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) return;

    int blockX = static_cast<int>(floor(position.x)) % 16;
    int blockY = static_cast<int>(floor(position.y));
    int blockZ = static_cast<int>(floor(position.z)) % 16;
    if (blockX < 0) blockX += 16;
    if (blockZ < 0) blockZ += 16;

    chunk->setBlock(blockX, blockY, blockZ, blockId);
    asyncSaveChunk(*chunk);
}

void World::tick() {
    worldAge++;
    timeOfDay = (timeOfDay + 1) % 24000;

    // Process completed chunks from worker threads
    chunk_manager_->processCompletedChunks();

    if (worldAge % 20 == 0) {
        updateChunkStatuses();
    }

    if (worldAge % 200 == 0) {
        unloadInactiveChunks();
    }

    auto chunks = chunk_manager_->getAllChunks();
    for (ChunkColumn* chunk : chunks) {
        if (chunk && chunk->status == ChunkStatus::TICKING) {
            // Chunk ticking logic here
        }
    }
}

std::vector<uint8_t> World::getDimension() {
    NbtBuilder nbt;
    nbt.startCompound();
        nbt.writeTagByte("bed_works", 1);
        nbt.writeTagByte("has_ceiling", 0);
        nbt.writeTagDouble("coordinate_scale", 1.0);
        nbt.writeTagByte("piglin_safe", 0);
        nbt.writeTagByte("has_skylight", 1);
        nbt.writeTagByte("ultrawarm", 0);
        nbt.writeTagString("infiniburn", "minecraft:infiniburn_overworld");
        nbt.writeTagString("effects", "minecraft:overworld");
        nbt.writeTagByte("has_raids", 1);
        nbt.writeTagFloat("ambient_light", 0.0f);
        nbt.writeTagInt("logical_height", 256);
        nbt.writeTagByte("natural", 1);
        nbt.writeTagByte("respawn_anchor_works", 0);
    nbt.endCompound();
    return nbt.buffer;
}

std::vector<uint8_t> World::getDimensionCodec() {
    NbtBuilder nbt;
    nbt.startCompound();
        nbt.startCompound("minecraft:dimension_type");
            nbt.writeTagString("type", "minecraft:dimension_type");
            nbt.startList("value", TAG_COMPOUND, 4);
                nbt.startListItem();
                    nbt.writeTagString("name", "minecraft:overworld");
                    nbt.writeTagInt("id", 0);
                    nbt.startCompound("element");
                        nbt.writeTagByte("bed_works", 1);
                        nbt.writeTagByte("has_ceiling", 0);
                        nbt.writeTagDouble("coordinate_scale", 1.0);
                        nbt.writeTagByte("piglin_safe", 0);
                        nbt.writeTagByte("has_skylight", 1);
                        nbt.writeTagByte("ultrawarm", 0);
                        nbt.writeTagString("infiniburn", "minecraft:infiniburn_overworld");
                        nbt.writeTagString("effects", "minecraft:overworld");
                        nbt.writeTagByte("has_raids", 1);
                        nbt.writeTagFloat("ambient_light", 0.0f);
                        nbt.writeTagInt("logical_height", 256);
                        nbt.writeTagByte("natural", 1);
                        nbt.writeTagByte("respawn_anchor_works", 0);
                    nbt.endCompound();
                nbt.endListItem();

                nbt.startListItem();
                    nbt.writeTagString("name", "minecraft:overworld_caves");
                    nbt.writeTagInt("id", 1);
                    nbt.startCompound("element");
                        nbt.writeTagByte("bed_works", 1);
                        nbt.writeTagByte("has_ceiling", 1);
                        nbt.writeTagDouble("coordinate_scale", 1.0);
                        nbt.writeTagByte("piglin_safe", 0);
                        nbt.writeTagByte("has_skylight", 1);
                        nbt.writeTagByte("ultrawarm", 0);
                        nbt.writeTagString("infiniburn", "minecraft:infiniburn_overworld");
                        nbt.writeTagString("effects", "minecraft:overworld");
                        nbt.writeTagByte("has_raids", 1);
                        nbt.writeTagFloat("ambient_light", 0.0f);
                        nbt.writeTagInt("logical_height", 256);
                        nbt.writeTagByte("natural", 1);
                        nbt.writeTagByte("respawn_anchor_works", 0);
                    nbt.endCompound();
                nbt.endListItem();

                nbt.startListItem();
                    nbt.writeTagString("name", "minecraft:the_nether");
                    nbt.writeTagInt("id", 2);
                    nbt.startCompound("element");
                        nbt.writeTagString("infiniburn", "minecraft:infiniburn_nether");
                        nbt.writeTagString("effects", "minecraft:the_nether");
                        nbt.writeTagByte("ultrawarm", 1);
                        nbt.writeTagInt("logical_height", 128);
                        nbt.writeTagByte("natural", 0);
                        nbt.writeTagByte("bed_works", 0);
                        nbt.writeTagLong("fixed_time", 18000);
                        nbt.writeTagDouble("coordinate_scale", 8.0);
                        nbt.writeTagByte("piglin_safe", 1);
                        nbt.writeTagByte("has_skylight", 0);
                        nbt.writeTagByte("has_ceiling", 1);
                        nbt.writeTagFloat("ambient_light", 0.1f);
                        nbt.writeTagByte("has_raids", 0);
                        nbt.writeTagByte("respawn_anchor_works", 1);
                    nbt.endCompound();
                nbt.endListItem();

                nbt.startListItem();
                    nbt.writeTagString("name", "minecraft:the_end");
                    nbt.writeTagInt("id", 3);
                    nbt.startCompound("element");
                        nbt.writeTagString("infiniburn", "minecraft:infiniburn_end");
                        nbt.writeTagString("effects", "minecraft:the_end");
                        nbt.writeTagByte("ultrawarm", 0);
                        nbt.writeTagInt("logical_height", 256);
                        nbt.writeTagByte("natural", 0);
                        nbt.writeTagByte("bed_works", 0);
                        nbt.writeTagLong("fixed_time", 6000);
                        nbt.writeTagDouble("coordinate_scale", 1.0);
                        nbt.writeTagByte("piglin_safe", 0);
                        nbt.writeTagByte("has_skylight", 0);
                        nbt.writeTagByte("has_ceiling", 0);
                        nbt.writeTagFloat("ambient_light", 0.0f);
                        nbt.writeTagByte("has_raids", 1);
                        nbt.writeTagByte("respawn_anchor_works", 0);
                    nbt.endCompound();
                nbt.endListItem();
        nbt.endCompound();

        nbt.startCompound("minecraft:worldgen/biome");
            nbt.writeTagString("type", "minecraft:worldgen/biome");
            nbt.startList("value", TAG_COMPOUND, 2);
                nbt.startListItem();
                    nbt.writeTagString("name", "minecraft:plains");
                    nbt.writeTagInt("id", 1);
                    nbt.startCompound("element");
                        nbt.writeTagString("precipitation", "rain");
                        nbt.writeTagString("category", "plains");
                        nbt.writeTagFloat("depth", 0.125f);
                        nbt.writeTagFloat("scale", 0.05f);
                        nbt.writeTagFloat("temperature", 0.8f);
                        nbt.writeTagFloat("downfall", 0.4f);
                        nbt.startCompound("effects");
                            nbt.writeTagInt("sky_color", 7907327);
                            nbt.writeTagInt("water_fog_color", 329011);
                            nbt.writeTagInt("fog_color", 12638463);
                            nbt.writeTagInt("water_color", 4159204);
                            nbt.startCompound("mood_sound");
                                nbt.writeTagDouble("offset", 2.0);
                                nbt.writeTagString("sound", "minecraft:ambient.cave");
                                nbt.writeTagInt("block_search_extent", 8);
                                nbt.writeTagInt("tick_delay", 6000);
                            nbt.endCompound();
                        nbt.endCompound();
                    nbt.endCompound();
                nbt.endListItem();

                nbt.startListItem();
                    nbt.writeTagString("name", "minecraft:ocean");
                    nbt.writeTagInt("id", 0);
                    nbt.startCompound("element");
                        nbt.writeTagString("precipitation", "rain");
                        nbt.writeTagString("category", "ocean");
                        nbt.writeTagFloat("depth", -1.0f);
                        nbt.writeTagFloat("scale", 0.1f);
                        nbt.writeTagFloat("temperature", 0.5f);
                        nbt.writeTagFloat("downfall", 0.5f);
                        nbt.startCompound("effects");
                            nbt.writeTagInt("sky_color", 8103167);
                            nbt.writeTagInt("water_fog_color", 329011);
                            nbt.writeTagInt("fog_color", 12638463);
                            nbt.writeTagInt("water_color", 4159204);
                            nbt.startCompound("mood_sound");
                                nbt.writeTagDouble("offset", 2.0);
                                nbt.writeTagString("sound", "minecraft:ambient.cave");
                                nbt.writeTagInt("block_search_extent", 8);
                                nbt.writeTagInt("tick_delay", 6000);
                            nbt.endCompound();
                        nbt.endCompound();
                    nbt.endCompound();
                nbt.endListItem();
        nbt.endCompound();
    nbt.endCompound();
    return nbt.buffer;
}

int64_t World::getWorldAge() const {
    return worldAge;
}

int64_t World::getTimeOfDay() const {
    return timeOfDay;
}

void World::setTimeOfDay(int64_t time) {
    timeOfDay = time % 24000;
}
