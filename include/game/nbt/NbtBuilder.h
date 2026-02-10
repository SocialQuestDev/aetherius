#pragma once
#include <vector>
#include <string>
#include <cstdint>

enum TagType {
    TAG_END, TAG_BYTE, TAG_SHORT, TAG_INT, TAG_LONG, TAG_FLOAT,
    TAG_DOUBLE, TAG_BYTE_ARRAY, TAG_STRING, TAG_LIST, TAG_COMPOUND,
    TAG_INT_ARRAY, TAG_LONG_ARRAY
};

class NbtBuilder {
public:
    std::vector<uint8_t> buffer;

    void writeByte(int8_t v);
    void writeShort(int16_t v);
    void writeInt(int32_t v);
    void writeLong(int64_t v);
    void writeULong(uint64_t v);
    void writeFloat(float v);
    void writeDouble(double v);
    void writeString(const std::string& s);

    void startCompound(const std::string& name);
    void startCompound();
    void endCompound();

    void writeTagString(const std::string& name, const std::string& value);
    void writeTagInt(const std::string& name, int32_t value);
    void writeTagByte(const std::string& name, int8_t value);
    void writeTagLong(const std::string& name, int64_t value);
    void writeTagFloat(const std::string& name, float value);
    void writeTagDouble(const std::string& name, double value);
    void writeLongArray(const std::string& name, const std::vector<int64_t>& data);

    void startList(const std::string& name, TagType type, int32_t count);
    void startListItem();
    void endListItem();

    size_t getReaderIndex() const;
    void setReaderIndex(size_t index);

    int8_t readByte();
    int16_t readShort();
    int32_t readInt();
    int64_t readLong();
    float readFloat();
    std::string readString();

    uint8_t readTagType();
    void readStartCompound();
    void readEndCompound();
    void readStartList(TagType& type, int32_t& count);

    std::string readTagString();
    int32_t readTagInt();
    int8_t readTagByte();
    int64_t readTagLong();
    float readTagFloat();

private:
    size_t readerIndex = 0;
    int compound_level = 0;
};