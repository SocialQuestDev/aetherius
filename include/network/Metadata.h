#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <optional>
#include "auth/UUID.h"
#include "other/Vector3.h"

class PacketBuffer;

class Metadata {
public:
    void addByte(uint8_t index, uint8_t value);
    void addVarInt(uint8_t index, int32_t value);
    void addFloat(uint8_t index, float value);
    void addString(uint8_t index, const std::string& value);
    void addChat(uint8_t index, const std::string& json);
    void addOptChat(uint8_t index, const std::optional<std::string>& json);
    void addSlot(uint8_t index, bool present, int32_t itemId = 0, uint8_t itemCount = 0, const std::vector<uint8_t>& nbt = {});
    void addBoolean(uint8_t index, bool value);
    void addRotation(uint8_t index, float x, float y, float z);
    void addPosition(uint8_t index, int32_t x, int32_t y, int32_t z);
    void addOptPosition(uint8_t index, const std::optional<Vector3>& pos);
    void addDirection(uint8_t index, int32_t direction);
    void addOptUUID(uint8_t index, const std::optional<UUID>& uuid);
    void addOptBlockID(uint8_t index, const std::optional<int32_t>& blockId);
    void addNBT(uint8_t index, const std::vector<uint8_t>& nbt);
    void addParticle(uint8_t index, int32_t particleId, const std::vector<uint8_t>& particleData);
    void addVillagerData(uint8_t index, int32_t type, int32_t profession, int32_t level);
    void addOptVarInt(uint8_t index, const std::optional<int32_t>& value);
    void addPose(uint8_t index, int32_t poseId);

    void write(PacketBuffer& buffer) const;
    void clear();

private:
    std::vector<uint8_t> data;

    void writeVarInt(int32_t value);
    uint64_t encodePosition(int32_t x, int32_t y, int32_t z) const;
};
