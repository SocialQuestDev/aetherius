#pragma once
#include <vector>

#include "../nbt/NbtBuilder.h"
#include "../../network/PacketBuffer.h"

class ChunkSection {
public:
    std::vector<uint8_t> serialize() {
        PacketBuffer buf;

        buf.writeShort(4096);

        uint8_t bitsPerBlock = 15;
        buf.data.push_back(bitsPerBlock);

        int dataArraySize = 960;
        buf.writeVarInt(dataArraySize);

        for(int i=0; i<dataArraySize; i++) {
            buf.writeLong(0);
        }

        return buf.data;
    }
};

class ChunkColumn {
public:
    int x, z;
    
    std::vector<uint8_t> serialize() {
        PacketBuffer buf;
        buf.writeInt(x);
        buf.writeInt(z);

        buf.writeVarInt(1); 

        NbtBuilder nbt;
        nbt.writeByte(TAG_COMPOUND); nbt.writeString("");
        nbt.startList("MOTION_BLOCKING", TAG_LONG, 37);
        for(int i=0; i<37; i++) nbt.writeLong(0);
        nbt.endCompound();
        
        auto nbtData = nbt.buffer;
        buf.data.insert(buf.data.end(), nbtData.begin(), nbtData.end());

        buf.writeVarInt(1024);
        for(int i=0; i<1024; i++) buf.writeVarInt(1);

        ChunkSection section;
        std::vector<uint8_t> sectionData = section.serialize();
        
        buf.writeVarInt(sectionData.size());
        buf.data.insert(buf.data.end(), sectionData.begin(), sectionData.end());

        buf.writeVarInt(0);

        return buf.data;
    }
};