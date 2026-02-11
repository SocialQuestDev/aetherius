#include "game/nbt/NbtBuilder.h"

#include <cstring>
#include <stdexcept>
#include <vector>

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

void NbtBuilder::writeULong(uint64_t v) {
    for (int i = 7; i >= 0; --i) writeByte((v >> (i * 8)) & 0xFF);
}

void NbtBuilder::writeFloat(float v) {
    uint32_t data;
    std::memcpy(&data, &v, sizeof(float));
    writeInt(data);
}

void NbtBuilder::writeDouble(double v) {
    uint64_t data;
    std::memcpy(&data, &v, sizeof(double));
    writeULong(data);
}

void NbtBuilder::writeString(const std::string &s) {
    writeShort((int16_t)s.size());
    for (char c : s) writeByte(c);
}

// compound write

void NbtBuilder::startCompound(const std::string &name) {
    writeByte(TAG_COMPOUND);
    writeString(name);
    compound_level++;
}

void NbtBuilder::endCompound() {
    if (compound_level == 0) {
        throw std::runtime_error("Unmatched endCompound() call");
    }
    writeByte(TAG_END);
    compound_level--;
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

void NbtBuilder::writeTagDouble(const std::string& name, double value) {
    writeByte(TAG_DOUBLE);
    writeString(name);
    writeDouble(value);
}

void NbtBuilder::writeLongArray(const std::string& name, const std::vector<int64_t>& data) {
    writeByte(TAG_LONG_ARRAY);
    writeString(name);
    writeInt(data.size());
    for (int64_t val : data) {
        writeLong(val);
    }
}

void NbtBuilder::startList(const std::string& name, TagType type, int32_t count) {
    writeByte(TAG_LIST);
    writeString(name);
    writeByte(type);
    writeInt(count);
}

void NbtBuilder::startListItem() {
    compound_level++;
}

void NbtBuilder::endListItem() {
    if (compound_level == 0) {
        throw std::runtime_error("Unmatched endListItem() call");
    }
    writeByte(TAG_END);
    compound_level--;
}

size_t NbtBuilder::getReaderIndex() const {
    return readerIndex;
}

void NbtBuilder::setReaderIndex(size_t index) {
    if (index > buffer.size()) throw std::runtime_error("NbtBuilder: readerIndex out of bounds");
    readerIndex = index;
}

// read methods...
int8_t NbtBuilder::readByte() {
    if (readerIndex >= buffer.size()) throw std::runtime_error("NbtBuilder: readByte out of bounds");
    return buffer[readerIndex++];
}

int16_t NbtBuilder::readShort() {
    if (readerIndex + 1 >= buffer.size()) throw std::runtime_error("NbtBuilder: readShort out of bounds");
    int16_t res = (static_cast<int16_t>(buffer[readerIndex]) << 8) |
                   static_cast<int16_t>(buffer[readerIndex + 1]);
    readerIndex += 2;
    return res;
}

int32_t NbtBuilder::readInt() {
    if (readerIndex + 3 >= buffer.size()) throw std::runtime_error("NbtBuilder: readInt out of bounds");
    int32_t res = 0;
    for (int i = 0; i < 4; ++i) {
        res <<= 8;
        res |= static_cast<uint8_t>(buffer[readerIndex++]);
    }
    return res;
}

int64_t NbtBuilder::readLong() {
    if (readerIndex + 7 >= buffer.size()) throw std::runtime_error("NbtBuilder: readLong out of bounds");
    int64_t res = 0;
    for (int i = 0; i < 8; ++i) {
        res <<= 8;
        res |= static_cast<uint8_t>(buffer[readerIndex++]);
    }
    return res;
}

float NbtBuilder::readFloat() {
    uint32_t data = readInt();
    float v;
    std::memcpy(&v, &data, sizeof(float));
    return v;
}

std::string NbtBuilder::readString() {
    int16_t len = readShort();
    if (readerIndex + len > buffer.size()) throw std::runtime_error("NbtBuilder: readString out of bounds");
    std::string s(buffer.begin() + readerIndex, buffer.begin() + readerIndex + len);
    readerIndex += len;
    return s;
}

uint8_t NbtBuilder::readTagType() {
    return readByte();
}

void NbtBuilder::readStartCompound() {
    uint8_t tag = readByte();
    if (tag != TAG_COMPOUND) throw std::runtime_error("Expected TAG_COMPOUND");
    readString(); // name
}

void NbtBuilder::readEndCompound() {
    uint8_t tag = readByte();
    if (tag != TAG_END) throw std::runtime_error("Expected TAG_END");
}

void NbtBuilder::readStartList(TagType& type, int32_t& count) {
    uint8_t tag = readByte();
    if (tag != TAG_LIST) throw std::runtime_error("Expected TAG_LIST");
    readString(); // name
    type = static_cast<TagType>(readByte());
    count = readInt();
}

std::string NbtBuilder::readTagString() {
    uint8_t tag = readByte();
    if (tag != TAG_STRING) throw std::runtime_error("Expected TAG_STRING");
    readString(); // name
    return readString();
}

int32_t NbtBuilder::readTagInt() {
    uint8_t tag = readByte();
    if (tag != TAG_INT) throw std::runtime_error("Expected TAG_INT");
    readString(); // name
    return readInt();
}

int8_t NbtBuilder::readTagByte() {
    uint8_t tag = readByte();
    if (tag != TAG_BYTE) throw std::runtime_error("Expected TAG_BYTE");
    readString(); // name
    return readByte();
}

int64_t NbtBuilder::readTagLong() {
    uint8_t tag = readByte();
    if (tag != TAG_LONG) throw std::runtime_error("Expected TAG_LONG");
    readString(); // name
    return readLong();
}

float NbtBuilder::readTagFloat() {
    uint8_t tag = readByte();
    if (tag != TAG_FLOAT) throw std::runtime_error("Expected TAG_FLOAT");
    readString(); // name
    return readFloat();
}