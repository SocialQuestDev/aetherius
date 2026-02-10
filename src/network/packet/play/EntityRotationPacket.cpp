#include "../../../../include/network/packet/play/EntityRotationPacket.h"
#include "../../../../include/network/PacketBuffer.h"

EntityRotationPacket::EntityRotationPacket(int entityId, float yaw, float pitch, bool onGround)
    : entityId(entityId), yaw(yaw), pitch(pitch), onGround(onGround) {}

void EntityRotationPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    buffer.writeByte((int)(yaw * 256.0f / 360.0f));
    buffer.writeByte((int)(pitch * 256.0f / 360.0f));
    buffer.writeBoolean(onGround);
}
