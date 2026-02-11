#include "network/packet/outbound/play/EntityPositionPacket.h"
#include "network/PacketBuffer.h"

EntityPositionPacket::EntityPositionPacket(int entityId)
    : entityId(entityId) {}

void EntityPositionPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
}
