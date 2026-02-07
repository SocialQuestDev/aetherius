#pragma once
#include <vector>
#include <array>

#include "../nbt/NbtBuilder.h"
#include "../../network/PacketBuffer.h"

class ChunkSection {
public:
    std::array<int, 4096> blocks{};

    ChunkSection() {
        blocks.fill(0);
    }

    std::vector<uint8_t> serialize() {
        PacketBuffer buf;
        short blockCount = 0;
        for (int block : blocks) {
            if (block != 0) {
                blockCount++;
            }
        }
        buf.writeShort(blockCount);

        if (blockCount == 0) {
            buf.writeByte(4);
            buf.writeVarInt(1);
            buf.writeVarInt(0);
            buf.writeVarInt(0);
        } else {
            uint8_t bitsPerBlock = 15;
            buf.writeByte(bitsPerBlock);

            buf.writeVarInt(0);

            int dataArraySize = (4096 * bitsPerBlock) / 64;
            buf.writeVarInt(dataArraySize);

            std::vector<uint64_t> dataArray(dataArraySize, 0);
            for (int i = 0; i < 4096; ++i) {
                int startBit = i * bitsPerBlock;
                int startIndex = startBit / 64;
                int endIndex = (startBit + bitsPerBlock - 1) / 64;
                int startBitInIndex = startBit % 64;

                uint64_t value = blocks[i];

                dataArray[startIndex] |= (value << startBitInIndex);
                if (startIndex != endIndex) {
                    dataArray[endIndex] |= (value >> (64 - startBitInIndex));
                }
            }

            for (uint64_t val : dataArray) {
                buf.writeULong(val);
            }
        }

        return buf.data;
    }
};

class ChunkColumn {
public:
    int x, z;
    std::array<ChunkSection, 16> sections{};

    void setBlock(int x, int y, int z, int blockId) {
        if (y < 0 || y >= 256) return;
        int sectionIndex = y / 16;
        int blockIndex = (y % 16) * 16 * 16 + z * 16 + x;
        sections[sectionIndex].blocks[blockIndex] = blockId;
    }

    std::vector<uint8_t> serialize() {
        PacketBuffer buf;
        buf.writeInt(x);
        buf.writeInt(z);
        buf.writeBoolean(true); // Full chunk

        int mask = 0;
        for (int i = 0; i < 16; ++i) {
            mask |= (1 << i);
        }
        buf.writeVarInt(mask);

        NbtBuilder nbt;
        nbt.startCompound();
        nbt.startList("MOTION_BLOCKING", TAG_LONG, 37);
        for(int i = 0; i < 37; ++i) {
            nbt.writeLong(0);
        }
        nbt.endCompound();
        
        auto nbtData = nbt.buffer;
        buf.data.insert(buf.data.end(), nbtData.begin(), nbtData.end());

        for(int i = 0; i < 1024; ++i) {
            buf.writeVarInt(1); // Plains
        }

        PacketBuffer sectionsBuffer;
        for (auto& section : sections) {
            auto sectionData = section.serialize();
            sectionsBuffer.data.insert(sectionsBuffer.data.end(), sectionData.begin(), sectionData.end());
        }
        buf.writeByteArray(sectionsBuffer.data);

        buf.writeVarInt(0); // Block entities

        return buf.data;
    }
};