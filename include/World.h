#pragma once
#include "NbtBuilder.h"

std::vector<uint8_t> getDimensionCodec() {
    NbtBuilder nbt;

    nbt.writeByte(TAG_COMPOUND); 
    nbt.writeString("");

    nbt.startList("minecraft:dimension_type", TAG_COMPOUND, 1);
        nbt.writeByte(TAG_COMPOUND);
            nbt.writeTagString("name", "minecraft:overworld");
            nbt.writeByte(TAG_INT); nbt.writeString("id"); nbt.writeInt(0);
            
            nbt.writeByte(TAG_COMPOUND); nbt.writeString("element");
                nbt.writeTagByte("piglin_safe", 0);
                nbt.writeTagByte("natural", 1);
                nbt.writeTagFloat("ambient_light", 0.0f);
                nbt.writeTagString("infiniburn", "minecraft:infiniburn_overworld");
                nbt.writeTagString("effects", "minecraft:overworld");
                nbt.writeTagFloat("ambient_light", 0.0f);
                nbt.writeTagInt("logical_height", 256);
                nbt.writeTagInt("coordinate_scale", 1);
                nbt.writeTagByte("has_raids", 1);
                nbt.writeTagByte("respawn_anchor_works", 0);
                nbt.writeTagByte("bed_works", 1);
                nbt.writeTagByte("has_skylight", 1);
                nbt.writeTagByte("has_ceiling", 0);
                nbt.writeTagByte("ultrawarm", 0);
            nbt.endCompound();
        nbt.endCompound();
    nbt.endCompound();

    nbt.startList("minecraft:worldgen/biome", TAG_COMPOUND, 1);
        nbt.writeByte(TAG_COMPOUND);
            nbt.writeTagString("name", "minecraft:plains");
            nbt.writeByte(TAG_INT); nbt.writeString("id"); nbt.writeInt(1);
            nbt.writeByte(TAG_COMPOUND); nbt.writeString("element");
                nbt.writeTagString("precipitation", "rain");
                nbt.writeTagString("category", "plains");
                nbt.writeTagFloat("depth", 0.125f);
                nbt.writeTagFloat("scale", 0.05f);
                nbt.writeTagFloat("temperature", 0.8f);
                nbt.writeTagFloat("downfall", 0.4f);
                nbt.writeByte(TAG_COMPOUND); nbt.writeString("effects");
                    nbt.writeTagInt("sky_color", 7907327);
                    nbt.writeTagInt("water_fog_color", 329011);
                    nbt.writeTagInt("fog_color", 12638463);
                    nbt.writeTagInt("water_color", 4159204);
                nbt.endCompound();
            nbt.endCompound();
        nbt.endCompound();
    nbt.endCompound();

    nbt.endCompound();
    return nbt.buffer;
}