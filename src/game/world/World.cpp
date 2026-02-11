#include "game/world/World.h"
#include "game/nbt/NbtBuilder.h"
#include "game/player/PlayerList.h"
#include "console/Logger.h"
#include <cmath>
#include <boost/asio/post.hpp>
#include <vector>
#include <set>
#include <thread>

World::World(std::unique_ptr<WorldGenerator> generator, const std::string& worldName)
    : generator(std::move(generator)),
      storage(std::make_unique<WorldStorage>(worldName)),
      thread_pool_(std::make_unique<boost::asio::thread_pool>(std::thread::hardware_concurrency())) {}

ChunkColumn* World::getChunk(int x, int z) {
    std::lock_guard<std::mutex> lock(chunks_mutex_);
    auto it = chunks.find({x, z});
    if (it != chunks.end()) {
        it->second.touch();
        return &it->second;
    }
    return nullptr;
}

void World::getOrGenerateChunk(int x, int z, std::function<void(ChunkColumn*)> callback) {
    if (auto* chunk = getChunk(x, z)) {
        callback(chunk);
        return;
    }

    boost::asio::post(*thread_pool_, [this, x, z, callback]() {
        if (auto* chunk = getChunk(x, z)) {
            callback(chunk);
            return;
        }

        std::lock_guard<std::mutex> gen_lock(generation_mutex_);
        if (auto* chunk = getChunk(x, z)) {
            callback(chunk);
            return;
        }

        auto chunk = std::make_unique<ChunkColumn>();
        chunk->x = x;
        chunk->z = z;

        if (storage->loadChunk(x, z, *chunk)) {
            std::lock_guard<std::mutex> lock(chunks_mutex_);
            auto [it, success] = chunks.emplace(std::make_pair(x, z), std::move(*chunk));
            it->second.touch();
            callback(&it->second);
        } else {
            generator->generateChunk(*chunk);
            asyncSaveChunk(*chunk);

            std::lock_guard<std::mutex> lock(chunks_mutex_);
            auto [it, success] = chunks.emplace(std::make_pair(x, z), std::move(*chunk));
            it->second.touch();
            callback(&it->second);
        }
    });
}

void World::asyncSaveChunk(const ChunkColumn& chunk) {
    auto chunk_copy = std::make_shared<ChunkColumn>(chunk);
    boost::asio::post(*thread_pool_, [this, chunk_copy]() {
        storage->saveChunk(*chunk_copy);
    });
}

void World::syncSaveAllChunks() {
    std::lock_guard<std::mutex> lock(chunks_mutex_);
    for (const auto& pair : chunks) {
        storage->saveChunk(pair.second);
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

    std::lock_guard<std::mutex> lock(chunks_mutex_);
    for (auto& [pos, chunk] : chunks) {
        if (active_chunks_coords.contains(pos)) {
            chunk.status = ChunkStatus::TICKING;
        } else {
            chunk.status = ChunkStatus::VISUAL;
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

    {
        std::lock_guard<std::mutex> lock(chunks_mutex_);
        for (auto const& [pos, chunk] : chunks) {
            if (!keep_alive_chunks.contains(pos)) {
                if (std::chrono::steady_clock::now() - chunk.last_accessed > unload_threshold) {
                    to_unload_coords.push_back(pos);
                }
            }
        }
    }

    if (!to_unload_coords.empty()) {
        for (const auto& pos : to_unload_coords) {
            boost::asio::post(*thread_pool_, [this, pos, &unload_threshold]() {
                ChunkColumn chunk_copy;
                bool should_erase = false;

                {
                    std::lock_guard<std::mutex> lock(chunks_mutex_);
                    auto it = chunks.find(pos);
                    if (it != chunks.end()) {
                        if (std::chrono::steady_clock::now() - it->second.last_accessed > unload_threshold) {
                            chunk_copy = it->second;
                            should_erase = true;
                        }
                    }
                }

                if (should_erase) {
                    storage->saveChunk(chunk_copy);

                    std::lock_guard<std::mutex> lock(chunks_mutex_);
                    chunks.erase(pos);
                }
            });
        }
        LOG_DEBUG("Scheduled " + std::to_string(to_unload_coords.size()) + " inactive chunks for unloading.");
    }
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
    asyncSaveChunk(*chunk); // Use async save
}

void World::tick() {
    worldAge++;
    timeOfDay = (timeOfDay + 1) % 24000;

    if (worldAge % 20 == 0) {
        updateChunkStatuses();
    }

    if (worldAge % 200 == 0) {
        unloadInactiveChunks();
    }

    std::lock_guard<std::mutex> lock(chunks_mutex_);
    for (auto& [pos, chunk] : chunks) {
        if (chunk.status == ChunkStatus::TICKING) {
            // TODO: Add entity processing, random block updates, etc.
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
    nbt.startCompound(); // Root
        nbt.startCompound("minecraft:dimension_type");
            nbt.writeTagString("type", "minecraft:dimension_type");
            nbt.startList("value", TAG_COMPOUND, 4);
                // overworld
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
                    nbt.endCompound(); // End element
                nbt.endListItem();

                // overworld_caves
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
                    nbt.endCompound(); // End element
                nbt.endListItem();

                // the_nether
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
                    nbt.endCompound(); // End element
                nbt.endListItem();

                // the_end
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
                    nbt.endCompound(); // End element
                nbt.endListItem();
        nbt.endCompound(); // End dimension_type compound

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
                        nbt.endCompound(); // End effects
                    nbt.endCompound(); // End element
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
                        nbt.endCompound(); // End effects
                    nbt.endCompound(); // End element
                nbt.endListItem();
        nbt.endCompound(); // End biome compound
    nbt.endCompound(); // End Root
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
