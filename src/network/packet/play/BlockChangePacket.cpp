#include "../../../../include/network/packet/play/BlockChangePacket.h"
#include "../../../../include/network/PacketBuffer.h"

BlockChangePacket::BlockChangePacket(const Vector3& position, int blockId)
    : position(position), blockId(blockId) {}

void BlockChangePacket::write(PacketBuffer& buffer) {
    long long x = (long long)position.x;
    long long y = (long long)position.y;
    long long z = (long long)position.z;

    long long packedPosition = ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF);

    buffer.writeLong(packedPosition);
    buffer.writeVarInt(blockId);
}
