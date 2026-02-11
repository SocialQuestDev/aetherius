#include "../../../../include/network/packet/play/EntityMetadataPacket.h"
#include "../../../../include/network/PacketBuffer.h"

EntityMetadataPacket::EntityMetadataPacket(int entityId, const Metadata& metadata)
    : entityId(entityId), metadata(metadata) {}

void EntityMetadataPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);
    metadata.write(buffer);
}
