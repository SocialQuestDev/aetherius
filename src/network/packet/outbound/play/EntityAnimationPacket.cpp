#include "network/packet/outbound/play/EntityAnimationPacket.h"
#include "network/PacketBuffer.h"

EntityAnimationPacket::EntityAnimationPacket(int entityId, int animation)
    : entityId(entityId), animation(animation) {}

void EntityAnimationPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    buffer.writeByte(animation);
}
