#include "../../../../include/network/packet/play/BrandPacket.h"

BrandPacket::BrandPacket(const std::string& brand) : brand(brand) {}

int BrandPacket::getPacketId() const {
    return 0x17;
}

void BrandPacket::write(PacketBuffer& buffer) {
    buffer.writeString("minecraft:brand");
    buffer.writeString(brand);
}
