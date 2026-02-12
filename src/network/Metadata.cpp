#include "network/Metadata.h"
#include "network/PacketBuffer.h"
#include <cstring>

void Metadata::addByte(uint8_t index, uint8_t value) {
    data.push_back(index);
    data.push_back(0);
    data.push_back(value);
}

void Metadata::addVarInt(uint8_t index, int32_t value) {
    data.push_back(index);
    data.push_back(1);
    writeVarInt(value);
}

void Metadata::addFloat(uint8_t index, float value) {
    data.push_back(index);
    data.push_back(2);

    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    data.push_back((bits >> 24) & 0xFF);
    data.push_back((bits >> 16) & 0xFF);
    data.push_back((bits >> 8) & 0xFF);
    data.push_back(bits & 0xFF);
}

void Metadata::addString(uint8_t index, const std::string& value) {
    data.push_back(index);
    data.push_back(3);
    writeVarInt(static_cast<int32_t>(value.length()));
    data.insert(data.end(), value.begin(), value.end());
}

void Metadata::addChat(uint8_t index, const std::string& json) {
    data.push_back(index);
    data.push_back(4);
    writeVarInt(static_cast<int32_t>(json.length()));
    data.insert(data.end(), json.begin(), json.end());
}

void Metadata::addOptChat(uint8_t index, const std::optional<std::string>& json) {
    data.push_back(index);
    data.push_back(5);
    if (json.has_value()) {
        data.push_back(1);
        writeVarInt(static_cast<int32_t>(json->length()));
        data.insert(data.end(), json->begin(), json->end());
    } else {
        data.push_back(0);
    }
}

void Metadata::addSlot(uint8_t index, bool present, int32_t itemId, uint8_t itemCount, const std::vector<uint8_t>& nbt) {
    data.push_back(index);
    data.push_back(6);
    if (present) {
        data.push_back(1);
        writeVarInt(itemId);
        data.push_back(itemCount);
        data.insert(data.end(), nbt.begin(), nbt.end());
    } else {
        data.push_back(0);
    }
}

void Metadata::addBoolean(uint8_t index, bool value) {
    data.push_back(index);
    data.push_back(7);
    data.push_back(value ? 1 : 0);
}

void Metadata::addRotation(uint8_t index, float x, float y, float z) {
    data.push_back(index);
    data.push_back(8);

    for (float f : {x, y, z}) {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(float));
        data.push_back((bits >> 24) & 0xFF);
        data.push_back((bits >> 16) & 0xFF);
        data.push_back((bits >> 8) & 0xFF);
        data.push_back(bits & 0xFF);
    }
}

void Metadata::addPosition(uint8_t index, int32_t x, int32_t y, int32_t z) {
    data.push_back(index);
    data.push_back(9);

    uint64_t encoded = encodePosition(x, y, z);
    for (int i = 56; i >= 0; i -= 8) {
        data.push_back((encoded >> i) & 0xFF);
    }
}

void Metadata::addOptPosition(uint8_t index, const std::optional<Vector3>& pos) {
    data.push_back(index);
    data.push_back(10);
    if (pos.has_value()) {
        data.push_back(1);
        uint64_t encoded = encodePosition(
            static_cast<int32_t>(pos->x),
            static_cast<int32_t>(pos->y),
            static_cast<int32_t>(pos->z)
        );
        for (int i = 56; i >= 0; i -= 8) {
            data.push_back((encoded >> i) & 0xFF);
        }
    } else {
        data.push_back(0);
    }
}

void Metadata::addDirection(uint8_t index, int32_t direction) {
    data.push_back(index);
    data.push_back(11);
    writeVarInt(direction);
}

void Metadata::addOptUUID(uint8_t index, const std::optional<UUID>& uuid) {
    data.push_back(index);
    data.push_back(12);
    if (uuid.has_value()) {
        data.push_back(1);
        uint64_t most = uuid->high;
        uint64_t least = uuid->low;
        for (int i = 56; i >= 0; i -= 8) {
            data.push_back((most >> i) & 0xFF);
        }
        for (int i = 56; i >= 0; i -= 8) {
            data.push_back((least >> i) & 0xFF);
        }
    } else {
        data.push_back(0);
    }
}

void Metadata::addOptBlockID(uint8_t index, const std::optional<int32_t>& blockId) {
    data.push_back(index);
    data.push_back(13);
    if (blockId.has_value()) {
        writeVarInt(*blockId);
    } else {
        writeVarInt(0);
    }
}

void Metadata::addNBT(uint8_t index, const std::vector<uint8_t>& nbt) {
    data.push_back(index);
    data.push_back(14);
    data.insert(data.end(), nbt.begin(), nbt.end());
}

void Metadata::addParticle(uint8_t index, int32_t particleId, const std::vector<uint8_t>& particleData) {
    data.push_back(index);
    data.push_back(15);
    writeVarInt(particleId);
    data.insert(data.end(), particleData.begin(), particleData.end());
}

void Metadata::addVillagerData(uint8_t index, int32_t type, int32_t profession, int32_t level) {
    data.push_back(index);
    data.push_back(16);
    writeVarInt(type);
    writeVarInt(profession);
    writeVarInt(level);
}

void Metadata::addOptVarInt(uint8_t index, const std::optional<int32_t>& value) {
    data.push_back(index);
    data.push_back(17);
    if (value.has_value()) {
        writeVarInt(*value + 1);
    } else {
        writeVarInt(0);
    }
}

void Metadata::addPose(uint8_t index, int32_t poseId) {
    data.push_back(index);
    data.push_back(18);
    writeVarInt(poseId);
}

void Metadata::write(PacketBuffer& buffer) const {
    buffer.writeBytes(data);
    buffer.writeByte(0xFF);
}

void Metadata::clear() {
    data.clear();
}

void Metadata::writeVarInt(int32_t value) {
    uint32_t uvalue = static_cast<uint32_t>(value);
    do {
        uint8_t temp = uvalue & 0x7F;
        uvalue >>= 7;
        if (uvalue != 0) {
            temp |= 0x80;
        }
        data.push_back(temp);
    } while (uvalue != 0);
}

uint64_t Metadata::encodePosition(int32_t x, int32_t y, int32_t z) const {
    return ((static_cast<uint64_t>(x) & 0x3FFFFFF) << 38) |
           ((static_cast<uint64_t>(z) & 0x3FFFFFF) << 12) |
           (static_cast<uint64_t>(y) & 0xFFF);
}
