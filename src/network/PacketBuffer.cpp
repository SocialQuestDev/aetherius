#include "../../include/network/PacketBuffer.h"
#include "../../include/crypto/AES.h"
#include "../../include/Logger.h"
#include "../../include/other/deflate.h"

// read

uint8_t PacketBuffer::readByte() {
    if (readerIndex >= data.size()) {
        throw std::runtime_error("Buffer overflow: trying to read beyond data size");
    }
    return data[readerIndex++];
}

std::vector<uint8_t> PacketBuffer::readByteArray() {
    int length = readVarInt();

    std::vector<uint8_t> result;

    for (int i = 0; i < length; i++)
        result.push_back(readByte());

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
    } while ((read & 0b10000000) != 0);
    return result;
}

std::string PacketBuffer::readString() {
    const int length = readVarInt();
    std::string str(data.begin() + readerIndex, data.begin() + readerIndex + length);
    readerIndex += length;
    return str;
}

int64_t PacketBuffer::readLong() {
    int64_t res = 0;
    for (int i = 0; i < 8; i++) {
        res = (res << 8) | data[readerIndex++];
    }
    return res;
}

uint64_t PacketBuffer::readULong() {
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

unsigned short PacketBuffer::readUShort() {
    unsigned short res = (data[readerIndex] << 8) | data[readerIndex + 1];
    readerIndex += 2;
    return res;
}

// write

void PacketBuffer::writeByte(uint8_t value) {
    data.push_back(value);
}

void PacketBuffer::writeByteArray(std::vector<uint8_t>& value) {
    const int length = value.size();

    writeVarInt(length);

    data.insert(data.end(), value.begin(), value.end());
}

void PacketBuffer::writeVarInt(int value) {
    do {
        auto temp = static_cast<uint8_t>(value & 0b01111111);
        value >>= 7;
        if (value != 0) {
            temp |= 0b10000000;
        }
        writeByte(temp);
    } while (value != 0);
}

void PacketBuffer::writeString(const std::string &str) {
    writeVarInt(str.length());
    for (char c : str) writeByte(c);
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

// other

std::vector<uint8_t> PacketBuffer::finalize(bool compressionEnabled, int threshold, CryptoState* crypto) {
    PacketBuffer body;

    if (threshold >= 0 && compressionEnabled) {
        if ((int)this->data.size() >= threshold) {
            std::vector<uint8_t> compressed = compressData(this->data);
            body.writeVarInt(this->data.size()); // Uncompressed Length
            body.data.insert(body.data.end(), compressed.begin(), compressed.end());
        } else {
            body.writeVarInt(0);
            body.data.insert(body.data.end(), this->data.begin(), this->data.end());
        }
    } else {
        body.data.insert(body.data.end(), this->data.begin(), this->data.end());
    }

    PacketBuffer finalPacket;
    finalPacket.writeVarInt(body.data.size());
    finalPacket.data.insert(finalPacket.data.end(), body.data.begin(), body.data.end());

    if (crypto) {
        aes::encrypt(*crypto, finalPacket.data.data(), finalPacket.data.size());
    }

    return finalPacket.data;
}