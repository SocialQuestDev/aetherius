#include "../../../../include/network/packet/play/JoinGamePacket.h"
#include "../../../../include/game/world/World.h"
#include "../../../../include/Server.h"

JoinGamePacket::JoinGamePacket(int entityId, World& world) : entityId(entityId), world(world) {}

void JoinGamePacket::write(PacketBuffer& buffer) {
    buffer.writeInt(entityId);
    buffer.writeBoolean(false); // Is hardcore
    buffer.writeByte(1); // Gamemode: Creative
    buffer.writeByte(255); // Previous Gamemode
    buffer.writeVarInt(1); // World Count
    buffer.writeString("minecraft:overworld"); // World Name

    buffer.writeNbt(world.getDimensionCodec());
    buffer.writeNbt(world.getDimension());

    buffer.writeString("minecraft:overworld"); // World Name
    buffer.writeLong(0); // Hashed seed
    buffer.writeVarInt(20); // Max Players
    buffer.writeVarInt(10); // View Distance
    buffer.writeBoolean(false); // Reduced Debug Info
    buffer.writeBoolean(true); // Enable respawn screen
    buffer.writeBoolean(false); // Is Debug
    buffer.writeBoolean(false); // Is Flat
}
