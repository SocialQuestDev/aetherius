#include "../../include/network/PacketBuffer.h"
#include "../../include/crypto/AES.h"
#include "../../include/Logger.h"
#include "../../include/other/deflate.h"
#include <cstdint>
#include <cstring>
#include <vector>

#include "../../include/other/Vector3.h"

// read methods...
uint8_t PacketBuffer::readByte() {
    if (readerIndex >= data.size()) {
        throw std::runtime_error("Buffer overflow: trying to read beyond data size");
    }
    return data[readerIndex++];
}

Vector3 PacketBuffer::readPosition() {
    int64_t val = readLong();
    int32_t x = val >> 38;
    int32_t y = val << 52 >> 52;
    int32_t z = val << 26 >> 38;
    return new Vector3(x, y, z);
}

bool PacketBuffer::readBoolean() {
    return readByte() != 0;
}

int16_t PacketBuffer::readShort() {
    if (readerIndex + 2 > data.size()) {
        throw std::runtime_error("Buffer overflow: readShort");
    }
    int16_t res = (data[readerIndex] << 8) | data[readerIndex + 1];
    readerIndex += 2;
    return res;
}

int32_t PacketBuffer::readInt() {
    if (readerIndex + 4 > data.size()) {
        throw std::runtime_error("Buffer overflow: readInt");
    }
    int32_t res = (static_cast<uint32_t>(data[readerIndex]) << 24) |
                  (static_cast<uint32_t>(data[readerIndex + 1]) << 16) |
                  (static_cast<uint32_t>(data[readerIndex + 2]) << 8) |
                  (static_cast<uint32_t>(data[readerIndex + 3]));
    readerIndex += 4;
    return res;
}

float PacketBuffer::readFloat() {
    if (readerIndex + 4 > data.size()) {
        throw std::runtime_error("Buffer overflow: readFloat");
    }

    uint32_t val = (static_cast<uint32_t>(data[readerIndex])     << 24) |
                   (static_cast<uint32_t>(data[readerIndex + 1]) << 16) |
                   (static_cast<uint32_t>(data[readerIndex + 2]) << 8)  |
                    static_cast<uint32_t>(data[readerIndex + 3]);

    readerIndex += 4;

    float res;
    std::memcpy(&res, &val, sizeof(float));
    return res;
}

double PacketBuffer::readDouble() {
    if (readerIndex + 8 > data.size()) {
        throw std::runtime_error("Buffer overflow: readDouble expecting Float64");
    }

    uint64_t val = 0;
    for (int i = 0; i < 8; i++) {
        val = (val << 8) | data[readerIndex++];
    }

    double tempRes;
    std::memcpy(&tempRes, &val, sizeof(double));

    return static_cast<double>(tempRes);
}

// Убедись, что readUShort у тебя такой же (Big Endian)
unsigned short PacketBuffer::readUShort() {
    if (readerIndex + 2 > data.size()) {
        throw std::runtime_error("Buffer overflow: readUShort");
    }
    unsigned short res = (data[readerIndex] << 8) | data[readerIndex + 1];
    readerIndex += 2;
    return res;
}

std::vector<uint8_t> PacketBuffer::readByteArray() {
    int length = readVarInt();
    std::vector<uint8_t> result;
    result.reserve(length);
    if (readerIndex + length > data.size()) {
        throw std::runtime_error("Buffer overflow: trying to read byte array beyond data size");
    }
    result.insert(result.end(), data.begin() + readerIndex, data.begin() + readerIndex + length);
    readerIndex += length;
    return result;
}

int PacketBuffer::readVarInt() {
    int numRead = 0;
    int result = 0;
    uint8_t read;
    do {
        read = readByte();
        int value = (read & 0b01111111);
        result |= (value << (7 * numRead));
        numRead++;
        if (numRead > 5) {
            throw std::runtime_error("VarInt is too big");
        }
    } while ((read & 0b10000000) != 0);
    return result;
}

std::string PacketBuffer::readString() {
    const int length = readVarInt();
    if (readerIndex + length > data.size()) {
        throw std::runtime_error("Buffer overflow: trying to read string beyond data size");
    }
    std::string str(data.begin() + readerIndex, data.begin() + readerIndex + length);
    readerIndex += length;
    return str;
}

int64_t PacketBuffer::readLong() {
    if (readerIndex + 8 > data.size()) {
        throw std::runtime_error("Buffer overflow: trying to read long beyond data size");
    }
    int64_t res = 0;
    for (int i = 0; i < 8; i++) {
        res = (res << 8) | data[readerIndex++];
    }
    return res;
}

uint64_t PacketBuffer::readULong() {
    if (readerIndex + 8 > data.size()) {
        throw std::runtime_error("Buffer overflow: trying to read ulong beyond data size");
    }
    uint64_t res = 0;
    for (int i = 0; i < 8; i++) {
        res = (res << 8) | data[readerIndex++];
    }
    return res;
}

UUID PacketBuffer::readUUID() {
    uint64_t high = readULong();
    uint64_t low = readULong();
    return {high, low};
}


// write methods...
void PacketBuffer::writeByte(uint8_t value) { data.push_back(value); }
void PacketBuffer::writeBoolean(bool value) { writeByte(value ? 1 : 0); }

void PacketBuffer::writeByteArray(std::vector<uint8_t>& value) {
    writeVarInt(value.size());
    data.insert(data.end(), value.begin(), value.end());
}

void PacketBuffer::writeVarInt(int value) {
    uint32_t uValue = static_cast<uint32_t>(value); 
    do {
        uint8_t temp = static_cast<uint8_t>(uValue & 0b01111111);
        uValue >>= 7;
        if (uValue != 0) {
            temp |= 0b10000000;
        }
        writeByte(temp);
    } while (uValue != 0);
}


void PacketBuffer::writeString(const std::string &str) {
    writeVarInt(str.length());
    data.insert(data.end(), str.begin(), str.end());
}

void PacketBuffer::writeShort(int16_t value) {
    writeByte((value >> 8) & 0xFF);
    writeByte(value & 0xFF);
}

void PacketBuffer::writeInt(int32_t value) {
    writeByte((value >> 24) & 0xFF);
    writeByte((value >> 16) & 0xFF);
    writeByte((value >> 8) & 0xFF);
    writeByte(value & 0xFF);
}

void PacketBuffer::writeLong(int64_t value) {
    for (int i = 7; i >= 0; i--) {
        writeByte((value >> (i * 8)) & 0xFF);
    }
}

void PacketBuffer::writeULong(uint64_t value) {
    for(int i=7; i>=0; i--) writeByte((value >> (i*8)) & 0xFF);
}

void PacketBuffer::writeUUID(uint64_t high, uint64_t low) {
    writeULong(high);
    writeULong(low);
}

void PacketBuffer::writeUUID(const UUID &uuid) {
    writeUUID(uuid.high, uuid.low);
}

void PacketBuffer::writeFloat(float value) {
    uint32_t val;
    std::memcpy(&val, &value, sizeof(float));
    writeInt(val);
}

void PacketBuffer::writeDouble(double value) {
    uint64_t val;
    std::memcpy(&val, &value, sizeof(double));
    writeULong(val);
}

void PacketBuffer::writeNbt(const std::vector<uint8_t>& nbtData) {
    for (uint8_t b : nbtData) {
        this->data.push_back(b);
    }
}

// other

std::vector<uint8_t> PacketBuffer::finalize(bool compressionEnabled, int threshold, CryptoState* crypto) {
    PacketBuffer payload_buf; // This will hold the data part of the packet ([Data Length] + [Data])

    if (compressionEnabled) {
        if ((int)this->data.size() >= threshold) {
            // Data is large enough to be compressed
            std::vector<uint8_t> compressed = compressData(this->data);

            // Write the size of the *uncompressed* data
            payload_buf.writeVarInt(this->data.size());
            // Append the *compressed* data
            payload_buf.data.insert(payload_buf.data.end(), compressed.begin(), compressed.end());
        } else {
            // Data is too small, send uncompressed
            // Write 0 as data length to indicate uncompressed packet
            payload_buf.writeVarInt(0);
            // Append the uncompressed data
            payload_buf.data.insert(payload_buf.data.end(), this->data.begin(), this->data.end());
        }
    } else {
        // Compression is not enabled at all for this connection
        payload_buf.data = this->data;
    }

    // Now, create the final packet by prepending the total length of the payload
    PacketBuffer final_packet_buf;
    final_packet_buf.writeVarInt(payload_buf.data.size());
    final_packet_buf.data.insert(final_packet_buf.data.end(), payload_buf.data.begin(), payload_buf.data.end());

    std::vector<uint8_t> final_data = final_packet_buf.data;

    if (crypto) {
        aes::encrypt(*crypto, final_data.data(), final_data.size());
    }

    return final_data;
}