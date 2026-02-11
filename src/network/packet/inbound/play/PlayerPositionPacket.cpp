#include "network/packet/inbound/play/PlayerPositionPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/EntityTeleportPacket.h"
#include "network/packet/outbound/play/EntityPositionPacket.h"
#include "network/packet/outbound/play/EntityPositionAndRotationPacket.h"
#include "network/packet/outbound/play/EntityRotationPacket.h"
#include "network/packet/outbound/play/EntityHeadLookPacket.h"

// PlayerPositionPacket
void PlayerPositionPacket::read(PacketBuffer& buffer) {
    x = buffer.readDouble();
    y = buffer.readDouble();
    z = buffer.readDouble();
    onGround = buffer.readBoolean();
}

void PlayerPositionPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setPosition(Vector3(x, y, z));
        player->setOnGround(onGround);
        if (!player->isDead() && y < -10.0) {
            player->kill();
        }

        EntityTeleportPacket packet(player->getId(), player->getPosition(), player->getRotation().x, player->getRotation().y, player->isOnGround());
        EntityHeadLookPacket headLook(player->getId(), player->getRotation().x);
        for (const auto& other : PlayerList::getInstance().getPlayers()) {
            if (other->getId() != player->getId()) {
                other->getConnection()->send_packet(packet);
                other->getConnection()->send_packet(headLook);
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

        EntityTeleportPacket teleport(player->getId(), player->getPosition(), player->getRotation().x, player->getRotation().y, player->isOnGround());
        EntityHeadLookPacket headLook(player->getId(), player->getRotation().x);

        for (const auto& other : PlayerList::getInstance().getPlayers()) {
            if (other->getId() != player->getId()) {
                other->getConnection()->send_packet(teleport);
                other->getConnection()->send_packet(headLook);
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

        EntityTeleportPacket teleport(player->getId(), player->getPosition(), player->getRotation().x, player->getRotation().y, player->isOnGround());
        EntityHeadLookPacket headLook(player->getId(), player->getRotation().x);

        for (const auto& other : PlayerList::getInstance().getPlayers()) {
            if (other->getId() != player->getId()) {
                other->getConnection()->send_packet(teleport);
                other->getConnection()->send_packet(headLook);
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
