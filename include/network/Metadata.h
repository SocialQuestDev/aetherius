#pragma once

#include <vector>
#include <cstdint>
#include <string>

class PacketBuffer; // Forward declaration

class Metadata {
public:
    // Adds a metadata entry of type Byte
    void addByte(uint8_t index, uint8_t value);

    // Adds a metadata entry of type VarInt
    void addVarInt(uint8_t index, int value);

    // Adds a metadata entry of type Pose
    void addPose(uint8_t index, int poseId);

    // Writes the collected metadata to a buffer
    void write(PacketBuffer& buffer) const;

private:
    std::vector<uint8_t> data;
};
