#include "../../../../include/network/packet/play/BlockDigResponsePacket.h"
#include "../../../../include/network/PacketBuffer.h"

BlockDigResponsePacket::BlockDigResponsePacket(int entityId, const Vector3& position, char destroyStage)
    : entityId(entityId), position(position), destroyStage(destroyStage) {}

void BlockDigResponsePacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    long long packedPosition = ((long long)(int)position.x & 0x3FFFFFF) << 38 | ((long long)(int)position.z & 0x3FFFFFF) << 12 | ((long long)(int)position.y & 0xFFF);
    buffer.writeLong(packedPosition);
    buffer.writeByte(destroyStage);
}
