#include "../../../../include/network/packet/play/EntityStatusPacket.h"

EntityStatusPacket::EntityStatusPacket(int entityId, char status)
    : entityId(entityId), status(status) {}

void EntityStatusPacket::write(PacketBuffer& buffer) {
    buffer.writeInt(entityId);
    buffer.writeByte(status);
}
