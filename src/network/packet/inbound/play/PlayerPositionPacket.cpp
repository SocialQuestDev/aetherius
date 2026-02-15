#include "network/packet/inbound/play/PlayerPositionPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/EntityTeleportPacket.h"
#include "network/packet/outbound/play/EntityPositionPacket.h"
#include "network/packet/outbound/play/EntityPositionAndRotationPacket.h"
#include "network/packet/outbound/play/EntityRotationPacket.h"
#include "network/packet/outbound/play/EntityHeadLookPacket.h"
#include "Server.h"

void PlayerPositionPacket::read(PacketBuffer& buffer) {
    x = buffer.readDouble();
    y = buffer.readDouble();
    z = buffer.readDouble();
    onGround = buffer.readBoolean();
}

void PlayerPositionPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const double px = x;
    const double py = y;
    const double pz = z;
    const bool on_ground = onGround;
    Server::get_instance().post_game_task([self, px, py, pz, on_ground]() {
        auto player = self->getPlayer();
        if (player) {
            player->setPosition(Vector3(px, py, pz));
            player->setOnGround(on_ground);
            if (!player->isDead() && py < -10.0) {
                player->kill();
            }

            self->update_chunks();

            EntityTeleportPacket packet(player->getId(), player->getPosition(), player->getRotation().x, player->getRotation().y, player->isOnGround());
            EntityHeadLookPacket headLook(player->getId(), player->getRotation().x);
            for (const auto& other : PlayerList::getInstance().getPlayers()) {
                if (other->getId() != player->getId()) {
                    other->getConnection()->send_packet(packet);
                    other->getConnection()->send_packet(headLook);
                }
            }
        }
    });
}

void PlayerPositionAndRotationPacket::read(PacketBuffer& buffer) {
    x = buffer.readDouble();
    y = buffer.readDouble();
    z = buffer.readDouble();
    yaw = buffer.readFloat();
    pitch = buffer.readFloat();
    onGround = buffer.readBoolean();
}

void PlayerPositionAndRotationPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const double px = x;
    const double py = y;
    const double pz = z;
    const float ry = yaw;
    const float rp = pitch;
    const bool on_ground = onGround;
    Server::get_instance().post_game_task([self, px, py, pz, ry, rp, on_ground]() {
        auto player = self->getPlayer();
        if (player) {
            player->setPosition(Vector3(px, py, pz));
            player->setRotation(Vector2(ry, rp));
            player->setOnGround(on_ground);
            if (!player->isDead() && py < -10.0) {
                player->kill();
            }

            self->update_chunks();

            EntityTeleportPacket teleport(player->getId(), player->getPosition(), player->getRotation().x, player->getRotation().y, player->isOnGround());
            EntityHeadLookPacket headLook(player->getId(), player->getRotation().x);

            for (const auto& other : PlayerList::getInstance().getPlayers()) {
                if (other->getId() != player->getId()) {
                    other->getConnection()->send_packet(teleport);
                    other->getConnection()->send_packet(headLook);
                }
            }
        }
    });
}

void PlayerRotationPacket::read(PacketBuffer& buffer) {
    yaw = buffer.readFloat();
    pitch = buffer.readFloat();
    onGround = buffer.readBoolean();
}

void PlayerRotationPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const float ry = yaw;
    const float rp = pitch;
    const bool on_ground = onGround;
    Server::get_instance().post_game_task([self, ry, rp, on_ground]() {
        auto player = self->getPlayer();
        if (player) {
            player->setRotation(Vector2(ry, rp));
            player->setOnGround(on_ground);

            EntityTeleportPacket teleport(player->getId(), player->getPosition(), player->getRotation().x, player->getRotation().y, player->isOnGround());
            EntityHeadLookPacket headLook(player->getId(), player->getRotation().x);

            for (const auto& other : PlayerList::getInstance().getPlayers()) {
                if (other->getId() != player->getId()) {
                    other->getConnection()->send_packet(teleport);
                    other->getConnection()->send_packet(headLook);
                }
            }
        }
    });
}

void PlayerOnGroundPacket::read(PacketBuffer& buffer) {
    onGround = buffer.readBoolean();
}

void PlayerOnGroundPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const bool on_ground = onGround;
    Server::get_instance().post_game_task([self, on_ground]() {
        auto player = self->getPlayer();
        if (player) {
            player->setOnGround(on_ground);
        }
    });
}
