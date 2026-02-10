#include "../../../../include/network/packet/play/JoinGamePacket.h"
#include "../../../../include/game/world/World.h"
#include "../../../../include/Server.h"

JoinGamePacket::JoinGamePacket(int entityId, World& world) : entityId(entityId), world(world) {}

void JoinGamePacket::write(PacketBuffer& buffer) {
    auto& worldConfig = Server::get_instance().get_world_config();

    buffer.writeInt(entityId);
    buffer.writeBoolean(worldConfig["is_hardcore"].value_or(false));
    buffer.writeByte(worldConfig["gamemode"].value_or(0));
    buffer.writeByte(worldConfig["previous_gamemode"].value_or(255));
    buffer.writeVarInt(worldConfig["world_count"].value_or(1));
    buffer.writeString(worldConfig["world_name"].value_or("minecraft:overworld"));

    buffer.writeNbt(world.getDimensionCodec());
    buffer.writeNbt(world.getDimension());

    buffer.writeString(worldConfig["world_name"].value_or("minecraft:overworld"));
    buffer.writeLong(worldConfig["hashed_seed"].value_or(0LL));
    buffer.writeVarInt(Server::get_instance().get_config()["server"]["max_players"].value_or(20));
    buffer.writeVarInt(worldConfig["view_distance"].value_or(10));
    buffer.writeBoolean(worldConfig["reduced_debug_info"].value_or(false));
    buffer.writeBoolean(worldConfig["enable_respawn_screen"].value_or(true));
    buffer.writeBoolean(worldConfig["is_debug"].value_or(false));
    buffer.writeBoolean(worldConfig["is_flat"].value_or(true));
}
