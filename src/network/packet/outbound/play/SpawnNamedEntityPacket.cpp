#include "network/packet/outbound/play/SpawnNamedEntityPacket.h"
#include "game/player/Player.h"
#include "network/PacketBuffer.h"
#include "network/Metadata.h"

SpawnNamedEntityPacket::SpawnNamedEntityPacket(const std::shared_ptr<Player>& player) : player(player) {}

void SpawnNamedEntityPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(player->getId());
    buffer.writeUUID(player->getUuid());
    buffer.writeDouble(player->getPosition().x);
    buffer.writeDouble(player->getPosition().y);
    buffer.writeDouble(player->getPosition().z);
    buffer.writeByte(static_cast<int8_t>(player->getRotation().x * 256.0f / 360.0f));
    buffer.writeByte(static_cast<int8_t>(player->getRotation().y * 256.0f / 360.0f));

    // Initial Entity Metadata
    Metadata metadata;
    metadata.addByte(16, player->getDisplayedSkinParts()); // Skin parts

    uint8_t statusFlags = 0;
    if (player->isSneaking()) statusFlags |= 0x02;
    if (player->isSprinting()) statusFlags |= 0x08;
    metadata.addByte(0, statusFlags); // Status flags

    metadata.addPose(6, player->isSneaking() ? 5 : 0); // Pose

    metadata.addByte(7, static_cast<uint8_t>(player->getMainHand())); // Main hand

    metadata.write(buffer);
}
