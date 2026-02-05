#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

class PacketBuffer {
public:
    std::vector<uint8_t> data;
    size_t readerIndex = 0;

    explicit PacketBuffer(const std::vector<uint8_t>& buffer) : data(buffer) {}
    PacketBuffer() = default;
    
    uint8_t readByte() {
        if (readerIndex >= data.size()) {
            throw std::runtime_error("Buffer overflow: trying to read beyond data size");
        }
        return data[readerIndex++];
    }

    int readVarInt() {
        int numRead = 0;
        int result = 0;
        uint8_t read;
        do {
            read = readByte();
            int value = (read & 0b01111111);
            result |= (value << (7 * numRead));
            numRead++;
        } while ((read & 0b10000000) != 0);
        return result;
    }

    std::string readString() {
        const int length = readVarInt();
        std::string str(data.begin() + readerIndex, data.begin() + readerIndex + length);
        readerIndex += length;
        return str;
    }

    int64_t readLong() {
        int64_t res = 0;
        for (int i = 0; i < 8; i++) {
            res = (res << 8) | data[readerIndex++];
        }
        return res;
    }

    unsigned short readUShort() {
        unsigned short res = (data[readerIndex] << 8) | data[readerIndex + 1];
        readerIndex += 2;
        return res;
    }

    void writeVarInt(int value) {
        do {
            auto temp = static_cast<uint8_t>(value & 0b01111111);
            value >>= 7;
            if (value != 0) {
                temp |= 0b10000000;
            }
            data.push_back(temp);
        } while (value != 0);
    }

    void writeString(const std::string& str) {
        writeVarInt(str.length());
        for (char c : str) data.push_back(c);
    }

    void writeShort(int16_t value) {
        data.push_back((value >> 8) & 0xFF);
        data.push_back(value & 0xFF);
    }

    void writeInt(int32_t value) {
        data.push_back((value >> 24) & 0xFF);
        data.push_back((value >> 16) & 0xFF);
        data.push_back((value >> 8) & 0xFF);
        data.push_back(value & 0xFF);
    }

    void writeLong(int64_t value) {
        for (int i = 7; i >= 0; i--) {
            data.push_back((value >> (i * 8)) & 0xFF);
        }
    }

    void writeUUID(uint64_t high, uint64_t low) {
        for(int i=7; i>=0; i--) data.push_back((high >> (i*8)) & 0xFF);
        for(int i=7; i>=0; i--) data.push_back((low >> (i*8)) & 0xFF);
    }

    std::vector<uint8_t> finalize() {
        std::vector<uint8_t> result;
        PacketBuffer temp;
        temp.writeVarInt(data.size());
        
        result.insert(result.end(), temp.data.begin(), temp.data.end());
        result.insert(result.end(), data.begin(), data.end());
        return result;
    }
};
