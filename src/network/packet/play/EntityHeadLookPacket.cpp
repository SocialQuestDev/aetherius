#include "../../../../include/network/packet/play/EntityHeadLookPacket.h"
#include "../../../../include/network/PacketBuffer.h"

EntityHeadLookPacket::EntityHeadLookPacket(int entityId, float headYaw, float headPitch)
    : entityId(entityId), headYaw(headYaw), headPitch(headPitch) {}

void EntityHeadLookPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    buffer.writeByte(static_cast<int8_t>(headYaw * 256.0f / 360.0f));
}
