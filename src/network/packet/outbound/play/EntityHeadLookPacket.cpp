#include "network/packet/outbound/play/EntityHeadLookPacket.h"
#include "network/PacketBuffer.h"

EntityHeadLookPacket::EntityHeadLookPacket(int entityId, float headYaw)
    : entityId(entityId), headYaw(headYaw) {}

void EntityHeadLookPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    buffer.writeByte(static_cast<int8_t>(headYaw * 256.0f / 360.0f));
}
