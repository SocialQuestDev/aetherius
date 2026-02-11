#include "network/packet/outbound/login/SetCompressionPacket.h"

SetCompressionPacket::SetCompressionPacket(int threshold) : threshold(threshold) {}

void SetCompressionPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(threshold);
}
