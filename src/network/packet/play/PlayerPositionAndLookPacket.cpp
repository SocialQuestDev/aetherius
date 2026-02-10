#include "../../../../include/network/packet/play/PlayerPositionAndLookPacket.h"

PlayerPositionAndLookPacket::PlayerPositionAndLookPacket(double x, double y, double z, float yaw, float pitch, char flags, int teleportId)
    : x(x), y(y), z(z), yaw(yaw), pitch(pitch), flags(flags), teleportId(teleportId) {}

void PlayerPositionAndLookPacket::write(PacketBuffer& buffer) {
    buffer.writeDouble(x);
    buffer.writeDouble(y);
    buffer.writeDouble(z);
    buffer.writeFloat(yaw);
    buffer.writeFloat(pitch);
    buffer.writeByte(flags);
    buffer.writeVarInt(teleportId);
}