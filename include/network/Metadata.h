#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <optional>
#include "auth/UUID.h"
#include "other/Vector3.h"

class PacketBuffer; // Forward declaration

// Minecraft 1.16.2-1.16.5 (Protocol 751-752) Entity Metadata
class Metadata {
public:
    // Type 0: Byte
    void addByte(uint8_t index, uint8_t value);

    // Type 1: VarInt
    void addVarInt(uint8_t index, int32_t value);

    // Type 2: Float
    void addFloat(uint8_t index, float value);

    // Type 3: String
    void addString(uint8_t index, const std::string& value);

    // Type 4: Chat Component (JSON string)
    void addChat(uint8_t index, const std::string& json);

    // Type 5: OptChat (optional Chat Component)
    void addOptChat(uint8_t index, const std::optional<std::string>& json);

    // Type 6: Slot (item stack)
    void addSlot(uint8_t index, bool present, int32_t itemId = 0, uint8_t itemCount = 0, const std::vector<uint8_t>& nbt = {});

    // Type 7: Boolean
    void addBoolean(uint8_t index, bool value);

    // Type 8: Rotation (3 floats: x, y, z)
    void addRotation(uint8_t index, float x, float y, float z);

    // Type 9: Position (encoded block position)
    void addPosition(uint8_t index, int32_t x, int32_t y, int32_t z);

    // Type 10: OptPosition (optional block position)
    void addOptPosition(uint8_t index, const std::optional<Vector3>& pos);

    // Type 11: Direction (VarInt enum: Down=0, Up=1, North=2, South=3, West=4, East=5)
    void addDirection(uint8_t index, int32_t direction);

    // Type 12: OptUUID (optional UUID)
    void addOptUUID(uint8_t index, const std::optional<UUID>& uuid);

    // Type 13: OptBlockID (optional VarInt block state ID)
    void addOptBlockID(uint8_t index, const std::optional<int32_t>& blockId);

    // Type 14: NBT (Named Binary Tag)
    void addNBT(uint8_t index, const std::vector<uint8_t>& nbt);

    // Type 15: Particle
    void addParticle(uint8_t index, int32_t particleId, const std::vector<uint8_t>& particleData);

    // Type 16: Villager Data (VarInt type, VarInt profession, VarInt level)
    void addVillagerData(uint8_t index, int32_t type, int32_t profession, int32_t level);

    // Type 17: OptVarInt (optional VarInt, 0 = absent, value+1 = present)
    void addOptVarInt(uint8_t index, const std::optional<int32_t>& value);

    // Type 18: Pose (VarInt enum)
    void addPose(uint8_t index, int32_t poseId);

    // Writes the collected metadata to a buffer with 0xFF terminator
    void write(PacketBuffer& buffer) const;

    // Clears all metadata entries
    void clear();

private:
    std::vector<uint8_t> data;

    // Helper to write VarInt to internal buffer
    void writeVarInt(int32_t value);

    // Helper to encode block position
    uint64_t encodePosition(int32_t x, int32_t y, int32_t z) const;
};
