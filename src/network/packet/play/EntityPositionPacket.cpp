#include "../../../../include/network/packet/play/EntityPositionPacket.h"
#include "../../../../include/network/PacketBuffer.h"

EntityPositionPacket::EntityPositionPacket(int entityId)
    : entityId(entityId) {}

void EntityPositionPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
}
