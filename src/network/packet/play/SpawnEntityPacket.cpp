#include "../../../../include/network/packet/play/SpawnEntityPacket.h"
#include "../../../../include/network/PacketBuffer.h"

SpawnEntityPacket::SpawnEntityPacket(int entityId, UUID objectUUID, int type, const Vector3& position,
                                     float pitch, float yaw, int objectData,
                                     short velocityX, short velocityY, short velocityZ)
    : entityId(entityId), objectUUID(objectUUID), type(type), position(position),
      pitch(pitch), yaw(yaw), objectData(objectData),
      velocityX(velocityX), velocityY(velocityY), velocityZ(velocityZ) {}

void SpawnEntityPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    buffer.writeUUID(objectUUID);
    buffer.writeVarInt(type);
    buffer.writeDouble(position.x);
    buffer.writeDouble(position.y);
    buffer.writeDouble(position.z);
    buffer.writeByte((int)(pitch * 256.0f / 360.0f));
    buffer.writeByte((int)(yaw * 256.0f / 360.0f));
    buffer.writeInt(objectData);
    buffer.writeShort(velocityX);
    buffer.writeShort(velocityY);
    buffer.writeShort(velocityZ);
}
