#include "../../../../include/network/packet/play/EntityMetadataPacket.h"
#include "../../../../include/network/PacketBuffer.h"
#include "../../../../include/game/player/Player.h"

EntityMetadataPacket::EntityMetadataPacket(const Player& player)
    : entityId(player.getId()) {
    metadata = 0;
    if (player.isSneaking()) {
        metadata |= 0x02; // Bit 1 for sneaking
    }
    if (player.isSprinting()) {
        metadata |= 0x08; // Bit 3 for sprinting
    }
}

void EntityMetadataPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);

    // Index 0: Entity Flags (Byte)
    buffer.writeByte(0); // Index
    buffer.writeVarInt(0); // Type: Byte
    buffer.writeByte(metadata); // Value

    buffer.writeByte(0xFF); // End of metadata
}
