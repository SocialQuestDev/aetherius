#include "../../../../include/network/packet/play/PlayerPositionPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"

// PlayerPositionPacketFull
void PlayerPositionPacketFull::read(PacketBuffer& buffer) {
    x = buffer.readDouble();
    y = buffer.readDouble();
    z = buffer.readDouble();
    onGround = buffer.readBoolean();
}

void PlayerPositionPacketFull::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setPosition(x, y, z);
        player->setOnGround(onGround);
        if (!player->isDead() && y < -10.0) {
            player->kill();
        }
    }
}

// PlayerPositionAndRotationPacket
void PlayerPositionAndRotationPacket::read(PacketBuffer& buffer) {
    x = buffer.readDouble();
    y = buffer.readDouble();
    z = buffer.readDouble();
    yaw = buffer.readFloat();
    pitch = buffer.readFloat();
    onGround = buffer.readBoolean();
}

void PlayerPositionAndRotationPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setPosition(x, y, z);
        player->setRotation(yaw, pitch);
        player->setOnGround(onGround);
        if (!player->isDead() && y < -10.0) {
            player->kill();
        }
    }
}

// PlayerRotationPacket
void PlayerRotationPacket::read(PacketBuffer& buffer) {
    yaw = buffer.readFloat();
    pitch = buffer.readFloat();
    onGround = buffer.readBoolean();
}

void PlayerRotationPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setRotation(yaw, pitch);
        player->setOnGround(onGround);
    }
}

// PlayerOnGroundPacket
void PlayerOnGroundPacket::read(PacketBuffer& buffer) {
    onGround = buffer.readBoolean();
}

void PlayerOnGroundPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setOnGround(onGround);
    }
}
