#include "../../../../include/network/packet/play/EntityPositionAndRotationPacket.h"
#include "../../../../include/network/PacketBuffer.h"

EntityPositionAndRotationPacket::EntityPositionAndRotationPacket(int entityId, short deltaX, short deltaY, short deltaZ, float yaw, float pitch, bool onGround)
    : entityId(entityId), deltaX(deltaX), deltaY(deltaY), deltaZ(deltaZ), yaw(yaw), pitch(pitch), onGround(onGround) {}

void EntityPositionAndRotationPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    buffer.writeShort(deltaX);
    buffer.writeShort(deltaY);
    buffer.writeShort(deltaZ);
    buffer.writeByte((int)(yaw * 256.0f / 360.0f));
    buffer.writeByte((int)(pitch * 256.0f / 360.0f));
    buffer.writeBoolean(onGround);
}
