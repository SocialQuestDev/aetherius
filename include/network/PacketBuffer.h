#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include "../auth/UUID.h"
#include "../crypto/AES.h"

class PacketBuffer {
public:
    std::vector<uint8_t> data;
    size_t readerIndex = 0;

    explicit PacketBuffer(std::vector<uint8_t>& buffer) : data(buffer) {}
    PacketBuffer() = default;
    
    uint8_t readByte();

    std::vector<uint8_t> readByteArray();

    int readVarInt();

    std::string readString();

    int64_t readLong();

    uint64_t readULong();

    UUID readUUID();

    unsigned short readUShort();

    void writeByte(uint8_t value);

    void writeByteArray(std::vector<uint8_t>& value);

    void writeVarInt(int value);

    void writeString(const std::string& str);

    void writeShort(int16_t value);

    void writeInt(int32_t value);

    void writeLong(int64_t value);

    void writeULong(uint64_t value);

    void writeUUID(uint64_t high, uint64_t low);

    void writeUUID(const UUID& uuid);

    std::vector<uint8_t> finalize(bool compressionEnabled, int threshold, CryptoState* crypto);
};