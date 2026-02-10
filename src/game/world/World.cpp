#include "../../../include/game/world/World.h"
#include "../../../include/game/nbt/NbtBuilder.h"
#include <cmath>

World::World(std::unique_ptr<WorldGenerator> generator) : generator(std::move(generator)) {}

ChunkColumn* World::getChunk(int x, int z) {
    auto it = chunks.find({x, z});
    if (it != chunks.end()) {
        return &it->second;
    }
    return nullptr;
}

ChunkColumn* World::generateChunk(int x, int z) {
    ChunkColumn chunk;
    chunk.x = x;
    chunk.z = z;
    generator->generateChunk(chunk);
    auto [it, success] = chunks.emplace(std::make_pair(x, z), chunk);
    return &it->second;
}

int World::getBlock(const Vector3& position) {
    int chunkX = floor(position.x / 16.0);
    int chunkZ = floor(position.z / 16.0);

    ChunkColumn* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        return 0; // Air
    }

    int blockX = (int)floor(position.x) % 16;
    int blockY = (int)floor(position.y);
    int blockZ = (int)floor(position.z) % 16;
    if (blockX < 0) blockX += 16;
    if (blockZ < 0) blockZ += 16;

    return chunk->getBlock(blockX, blockY, blockZ);
}

void World::setBlock(const Vector3& position, int blockId) {
    int chunkX = floor(position.x / 16.0);
    int chunkZ = floor(position.z / 16.0);

    ChunkColumn* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        // For simplicity, we won't place blocks in unloaded chunks.
        // A full implementation might load or generate the chunk here.
        return;
    }

    int blockX = (int)floor(position.x) % 16;
    int blockY = (int)floor(position.y);
    int blockZ = (int)floor(position.z) % 16;
    if (blockX < 0) blockX += 16;
    if (blockZ < 0) blockZ += 16;

    chunk->setBlock(blockX, blockY, blockZ, blockId);
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
