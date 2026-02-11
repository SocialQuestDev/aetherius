#include "network/packet/outbound/play/DestroyEntitiesPacket.h"

DestroyEntitiesPacket::DestroyEntitiesPacket(const std::vector<int> &entities) : entityIds(entities) {}

void DestroyEntitiesPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityIds.capacity());
    for (const int entityId : entityIds) {
        buffer.writeVarInt(entityId);
    }
}
