#include "network/packet/outbound/play/EntityStatusPacket.h"

EntityStatusPacket::EntityStatusPacket(int entityId, std::byte status)
    : entityId(entityId), status(status) {}

void EntityStatusPacket::write(PacketBuffer& buffer) {
    buffer.writeInt(entityId);
    buffer.writeByte(static_cast<uint32_t>(status));
}
