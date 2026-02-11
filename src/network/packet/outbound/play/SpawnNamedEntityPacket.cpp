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
}
