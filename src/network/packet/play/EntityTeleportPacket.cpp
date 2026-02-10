#include "../../../../include/network/packet/play/EntityTeleportPacket.h"
#include "../../../../include/network/PacketBuffer.h"

EntityTeleportPacket::EntityTeleportPacket(int entityId, const Vector3& position, float yaw, float pitch, bool onGround)
    : entityId(entityId), position(position), yaw(yaw), pitch(pitch), onGround(onGround) {}

void EntityTeleportPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    buffer.writeDouble(position.x);
    buffer.writeDouble(position.y);
    buffer.writeDouble(position.z);
    buffer.writeByte((int)(yaw * 256.0f / 360.0f));
    buffer.writeByte((int)(pitch * 256.0f / 360.0f));
    buffer.writeBoolean(onGround);
}
