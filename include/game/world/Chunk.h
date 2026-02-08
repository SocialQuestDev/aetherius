#pragma once
#include <vector>
#include <array>
#include <string>
#include <cstdint>

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
            if (block != 0) blockCount++;
        }
        buf.writeShort(blockCount);

        // Используем 15 бит (Global Palette). 
        // В 1.16+ если BitsPerBlock >= 9, палитра (Palette) в пакете отсутствует.
        uint8_t bitsPerBlock = 15; 
        buf.writeByte(bitsPerBlock);

        int blocksPerLong = 4; // 64 / 15 = 4
        int dataArraySize = 1024; // 4096 / 4 = 1024
        
        buf.writeVarInt(dataArraySize);

        std::vector<uint64_t> dataArray(dataArraySize, 0);
        for (int i = 0; i < 4096; ++i) {
            uint64_t blockValue = (uint64_t)blocks[i];
            int longIndex = i / blocksPerLong; 
            int subIndex = i % blocksPerLong;
            int shift = subIndex * 15; // Каждые 15 бит

            dataArray[longIndex] |= (blockValue << shift);
        }

        for (uint64_t val : dataArray) {
            buf.writeULong(val);
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

    // Вспомогательная функция для записи Long Array в NBT (Исправлено)
    void writeNbtLongArray(std::vector<uint8_t>& buffer, const std::string& name, const std::vector<int64_t>& data) {
        buffer.push_back(12); // TAG_Long_Array
        
        // Имя тега
        uint16_t nameLen = (uint16_t)name.length();
        buffer.push_back((uint8_t)(nameLen >> 8));
        buffer.push_back((uint8_t)(nameLen & 0xFF));
        for (char c : name) buffer.push_back(c);

        // Размер массива
        int32_t size = (int32_t)data.size();
        buffer.push_back((uint8_t)((size >> 24) & 0xFF));
        buffer.push_back((uint8_t)((size >> 16) & 0xFF));
        buffer.push_back((uint8_t)((size >> 8) & 0xFF));
        buffer.push_back((uint8_t)(size & 0xFF));

        // Элементы
        for (int64_t val : data) {
            for (int i = 7; i >= 0; --i) {
                buffer.push_back((uint8_t)((val >> (i * 8)) & 0xFF));
            }
        }
    }

    std::vector<uint8_t> serialize() {
        PacketBuffer buf;
        
        // 1. Координаты
        buf.writeInt(x);
        buf.writeInt(z);
        buf.writeBoolean(true); 

        // 2. Маска секций
        int mask = 0;
        for (int i = 0; i < 16; ++i) {
            bool hasBlocks = false;
            for(int b : sections[i].blocks) if(b != 0) { hasBlocks = true; break; }
            if (hasBlocks) mask |= (1 << i);
        }
        if (mask == 0) mask = 1; 
        buf.writeVarInt(mask);

        // 3. NBT Heightmaps (Исправленный пустой компаунд)
        buf.writeByte(10); // TAG_Compound
        buf.writeShort(0); // Пустое имя (длина 0)
        buf.writeByte(0);  // TAG_End

        // 4. Биомы (Исправлено для 1.16.5)
        buf.writeVarInt(1024); // Длина массива
        for(int i = 0; i < 1024; ++i) {
            buf.writeVarInt(1); // Plains биом
        }

        // 5. Данные секций
        PacketBuffer sectionsData;
        for (int i = 0; i < 16; ++i) {
            if (mask & (1 << i)) {
                auto s = sections[i].serialize();
                sectionsData.data.insert(sectionsData.data.end(), s.begin(), s.end());
            }
        }
        
        buf.writeVarInt((int)sectionsData.data.size());
        buf.data.insert(buf.data.end(), sectionsData.data.begin(), sectionsData.data.end());

        // 6. Block Entities
        buf.writeVarInt(0); 

        return buf.data;
    }
};