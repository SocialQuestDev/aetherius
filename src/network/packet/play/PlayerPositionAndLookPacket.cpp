#include "../../../../include/network/packet/play/PlayerPositionAndLookPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/network/packet/play/EntityRotationPacket.h"
#include "../../../../include/network/packet/play/EntityHeadLookPacket.h"
#include "../../../../include/network/packet/play/EntityPositionAndRotationPacket.h"
#include "../../../../include/network/packet/play/EntityPositionPacket.h"

// Конструктор для исходящего пакета (сервер -> клиент)
PlayerPositionAndLookPacket::PlayerPositionAndLookPacket(double x, double y, double z, float yaw, float pitch, char flags, int teleportId)
    : x(x), y(y), z(z), yaw(yaw), pitch(pitch), flags(flags), teleportId(teleportId), onGround(false) {}

// Конструктор для входящего пакета (клиент -> сервер)
PlayerPositionAndLookPacket::PlayerPositionAndLookPacket() : x(0), y(0), z(0), yaw(0), pitch(0), flags(0), teleportId(0), onGround(false) {}

// Запись исходящего пакета
void PlayerPositionAndLookPacket::write(PacketBuffer& buffer) {
    buffer.writeDouble(x);
    buffer.writeDouble(y);
    buffer.writeDouble(z);
    buffer.writeFloat(yaw);
    buffer.writeFloat(pitch);
    buffer.writeByte(flags);
    buffer.writeVarInt(teleportId);
}

// Чтение входящего пакета
void PlayerPositionAndLookPacket::read(PacketBuffer& buffer) {
    this->x = buffer.readDouble();
    this->y = buffer.readDouble();
    this->z = buffer.readDouble();
    this->yaw = buffer.readFloat();
    this->pitch = buffer.readFloat();
    this->onGround = buffer.readBoolean();
}

// Обработка входящего пакета
void PlayerPositionAndLookPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (!player) return;

    // 1. Обновляем позицию и вращение самого игрока на сервере
    Vector3 oldPos = player->getPosition();
    player->setPosition({x, y, z});
    player->setRotation({pitch, yaw}); // pitch - X, yaw - Y

    // 2. Вычисляем дельту для пакетов относительного движения
    short deltaX = static_cast<short>((x * 32 - oldPos.x * 32) * 128);
    short deltaY = static_cast<short>((y * 32 - oldPos.y * 32) * 128);
    short deltaZ = static_cast<short>((z * 32 - oldPos.z * 32) * 128);

    // 3. Создаем пакеты для рассылки другим игрокам
    EntityPositionAndRotationPacket posRotPacket(player->getId(), deltaX, deltaY, deltaZ, yaw, pitch, onGround);
    EntityHeadLookPacket headLookPacket(player->getId(), yaw, pitch);

    // 4. Рассылаем пакеты всем остальным
    for (const auto& otherPlayer : PlayerList::getInstance().getPlayers()) {
        if (otherPlayer->getId() != player->getId()) {
            otherPlayer->getConnection()->send_packet(posRotPacket);
            otherPlayer->getConnection()->send_packet(headLookPacket);
        }
    }
}
