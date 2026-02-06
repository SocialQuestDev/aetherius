#include "../../../include/game/nbt/NbtBuilder.h"

#include <cstring>

// native write

void NbtBuilder::writeByte(int8_t v) { buffer.push_back(v); }

void NbtBuilder::writeShort(int16_t v) {
    writeByte((v >> 8) & 0xFF);
    writeByte(v & 0xFF);
}

void NbtBuilder::writeInt(int32_t v) {
    for (int i = 3; i >= 0; --i) writeByte((v >> (i * 8)) & 0xFF);
}

void NbtBuilder::writeLong(int64_t v) {
    for (int i = 7; i >= 0; --i) writeByte((v >> (i * 8)) & 0xFF);
}

void NbtBuilder::writeFloat(float v) {
    uint32_t data;
    std::memcpy(&data, &v, sizeof(float));
    writeInt(data);
}

void NbtBuilder::writeString(const std::string &s) {
    writeShort((int16_t)s.size());
    for (char c : s) writeByte(c);
}

// compound write

void NbtBuilder::startCompound(const std::string &name) {
    writeByte(TAG_COMPOUND);
    writeString(name);
}

void NbtBuilder::endCompound() {
    writeByte(TAG_END);
}

void NbtBuilder::startCompound() {
    startCompound("");
}

// tags write

void NbtBuilder::writeTagString(const std::string &name, const std::string &value) {
    writeByte(TAG_STRING);
    writeString(name);
    writeString(value);
}

void NbtBuilder::writeTagInt(const std::string& name, int32_t value) {
    writeByte(TAG_INT);
    writeString(name);
    writeInt(value);
}

void NbtBuilder::writeTagByte(const std::string& name, int8_t value) {
    writeByte(TAG_BYTE);
    writeString(name);
    writeByte(value);
}

void NbtBuilder::writeTagLong(const std::string& name, int64_t value) {
    writeByte(TAG_LONG);
    writeString(name);
    writeLong(value);
}

void NbtBuilder::writeTagFloat(const std::string& name, float value) {
    writeByte(TAG_FLOAT);
    writeString(name);
    writeFloat(value);
}

void NbtBuilder::startList(const std::string& name, TagType type, int32_t count) {
    writeByte(TAG_LIST);
    writeString(name);
    writeByte(type);
    writeInt(count);
}