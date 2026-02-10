#include "../../../../include/network/packet/play/PlayerPositionPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/network/packet/play/EntityTeleportPacket.h"

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
        player->setPosition(Vector3(x, y, z));
        player->setOnGround(onGround);
        if (!player->isDead() && y < -10.0) {
            player->kill();
        }

        // Broadcast the position update to other players
        EntityTeleportPacket packet(player->getId(), player->getPosition(), player->getRotation().x, player->getRotation().y, player->isOnGround());
        for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
            if (otherPlayer->getId() != player->getId()) {
                otherPlayer->getConnection()->send_packet(packet);
            }
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
        player->setPosition(Vector3(x, y, z));
        player->setRotation(Vector2(yaw, pitch));
        player->setOnGround(onGround);
        if (!player->isDead() && y < -10.0) {
            player->kill();
        }

        // Broadcast the position and rotation update to other players
        EntityTeleportPacket packet(player->getId(), player->getPosition(), player->getRotation().x, player->getRotation().y, player->isOnGround());
        for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
            if (otherPlayer->getId() != player->getId()) {
                otherPlayer->getConnection()->send_packet(packet);
            }
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
        player->setRotation(Vector2(yaw, pitch));
        player->setOnGround(onGround);

        // Broadcast the rotation update to other players
        // For rotation-only updates, we can use a different packet in the future,
        // but for now, EntityTeleportPacket will work.
        EntityTeleportPacket packet(player->getId(), player->getPosition(), player->getRotation().x, player->getRotation().y, player->isOnGround());
        for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
            if (otherPlayer->getId() != player->getId()) {
                otherPlayer->getConnection()->send_packet(packet);
            }
        }
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
