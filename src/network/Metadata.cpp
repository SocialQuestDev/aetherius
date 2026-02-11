#include "network/Metadata.h"
#include "network/PacketBuffer.h"

void Metadata::addByte(uint8_t index, uint8_t value) {
    data.push_back(index);
    data.push_back(0); // Type: Byte
    data.push_back(value);
}

void Metadata::addVarInt(uint8_t index, int value) {
    data.push_back(index);
    data.push_back(1); // Type: VarInt
    PacketBuffer temp;
    temp.writeVarInt(value);
    data.insert(data.end(), temp.data.begin(), temp.data.end());
}

void Metadata::addPose(uint8_t index, int poseId) {
    data.push_back(index);
    data.push_back(18); // Type: Pose
    PacketBuffer temp;
    temp.writeVarInt(poseId);
    data.insert(data.end(), temp.data.begin(), temp.data.end());
}

void Metadata::write(PacketBuffer& buffer) const {
    buffer.writeBytes(data);
    buffer.writeByte(0xFF); // End of metadata
}
