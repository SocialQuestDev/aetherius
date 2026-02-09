#include "../../../../include/network/packet/login/SetCompressionPacket.h"

SetCompressionPacket::SetCompressionPacket(int threshold) : threshold(threshold) {}

void SetCompressionPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(threshold);
}
