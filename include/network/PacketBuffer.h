#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include "../auth/UUID.h"
#include "../crypto/AES.h"

struct Vector3;

class PacketBuffer {
public:
    std::vector<uint8_t> data;
    size_t readerIndex = 0;

    explicit PacketBuffer(std::vector<uint8_t>& buffer) : data(buffer) {}
    PacketBuffer() = default;
    
    // Read methods
    uint8_t readByte();
    bool readBoolean();
    int16_t readShort();
    int32_t readInt();
    int64_t readLong();
    uint64_t readULong();
    float readFloat();
    double readDouble();
    int readVarInt();
    std::string readString();
    std::vector<uint8_t> readByteArray();
    UUID readUUID();
    unsigned short readUShort();
    Vector3 readPosition();

    // Write methods
    void writeByte(uint8_t value);
    void writeBoolean(bool value);
    void writeByteArray(std::vector<uint8_t>& value);
    void writeVarInt(int value);
    void writeString(const std::string& str);
    void writeShort(int16_t value);
    void writeInt(int32_t value);
    void writeLong(int64_t value);
    void writeULong(uint64_t value);
    void writeUUID(uint64_t high, uint64_t low);
    void writeUUID(const UUID& uuid);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeNbt(const std::vector<uint8_t>& nbt);

    std::vector<uint8_t> finalize(bool compressionEnabled, int threshold, CryptoState* crypto);
};
