#pragma once
#include <vector>
#include <string>
#include <cstdint>

enum TagType {
    TAG_END = 0,
    TAG_BYTE = 1,
    TAG_SHORT = 2,
    TAG_INT = 3,
    TAG_LONG = 4,
    TAG_FLOAT = 5,
    TAG_DOUBLE = 6,
    TAG_BYTE_ARRAY = 7,
    TAG_STRING = 8,
    TAG_LIST = 9,
    TAG_COMPOUND = 10
};

class NbtBuilder {
public:
    std::vector<uint8_t> buffer;

    void writeByte(int8_t v) { buffer.push_back(v); }
    
    void writeShort(int16_t v) {
        buffer.push_back((v >> 8) & 0xFF);
        buffer.push_back(v & 0xFF);
    }

    void writeInt(int32_t v) {
        for (int i = 3; i >= 0; --i) buffer.push_back((v >> (i * 8)) & 0xFF);
    }

    void writeLong(int64_t v) {
        for (int i = 7; i >= 0; --i) buffer.push_back((v >> (i * 8)) & 0xFF);
    }

    void writeFloat(float v) {
        uint32_t data;
        std::memcpy(&data, &v, sizeof(float));
        writeInt(data);
    }
    
    void writeString(const std::string& s) {
        writeShort((int16_t)s.size());
        for (char c : s) buffer.push_back(c);
    }

    void startCompound(const std::string& name) {
        writeByte(TAG_COMPOUND);
        writeString(name);
    }

    void endCompound() {
        writeByte(TAG_END);
    }

    void startCompound() {
    }

    void writeTagString(const std::string& name, const std::string& value) {
        writeByte(TAG_STRING);
        writeString(name);
        writeString(value);
    }

    void writeTagInt(const std::string& name, int32_t value) {
        writeByte(TAG_INT);
        writeString(name);
        writeInt(value);
    }
    
    void writeTagByte(const std::string& name, int8_t value) {
        writeByte(TAG_BYTE);
        writeString(name);
        writeByte(value);
    }
    
    void writeTagLong(const std::string& name, int64_t value) {
        writeByte(TAG_LONG);
        writeString(name);
        writeLong(value);
    }

    void writeTagFloat(const std::string& name, float value) {
        writeByte(TAG_FLOAT);
        writeString(name);
        writeFloat(value);
    }

    void startList(const std::string& name, TagType type, int32_t count) {
        writeByte(TAG_LIST);
        writeString(name);
        writeByte(type);
        writeInt(count);
    }
};