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

    void writeByte(int8_t v);
    
    void writeShort(int16_t v);

    void writeInt(int32_t v);

    void writeLong(int64_t v);

    void writeFloat(float v);
    
    void writeString(const std::string& s);

    void startCompound(const std::string& name);

    void endCompound();

    void startCompound();

    void writeTagString(const std::string& name, const std::string& value);

    void writeTagInt(const std::string& name, int32_t value);
    
    void writeTagByte(const std::string& name, int8_t value);
    
    void writeTagLong(const std::string& name, int64_t value);

    void writeTagFloat(const std::string& name, float value);

    void startList(const std::string& name, TagType type, int32_t count);
};