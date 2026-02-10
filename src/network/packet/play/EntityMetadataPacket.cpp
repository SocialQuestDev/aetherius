#include "../../../../include/network/packet/play/EntityMetadataPacket.h"
#include "../../../../include/network/PacketBuffer.h"
#include "../../../../include/game/player/Player.h"

EntityMetadataPacket::EntityMetadataPacket(const Player& player)
    : entityId(player.getId()) {
    metadata = 0;
    if (player.isSneaking()) {
        metadata |= 0x02; // Bit 1 for sneaking
    }
    if (player.isSprinting()) {
        metadata |= 0x08; // Bit 3 for sprinting
    }
}

void EntityMetadataPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(entityId);

    // --- Индекс 0: Флаги статуса ---
    buffer.writeByte(0);      // Индекс 0
    buffer.writeVarInt(0);    // Тип 0 (Byte)

    // Можно добавить огонь, если игрок горит: if (player.isOnFire()) flags |= 0x01;
    buffer.writeByte(metadata);

    // --- Индекс 6: Поза (Pose) ---
    // В 1.16.5 БЕЗ ЭТОГО игрок не присядет визуально (только ник опустится)
    buffer.writeByte(6);      // Индекс 6
    buffer.writeVarInt(18);   // Тип 18 (Pose)

    // Значения Pose: 0 - Standing, 5 - Sneaking
    if (metadata & 0x02) {
        buffer.writeVarInt(5); // Pose ID 5: Sneaking
    }
    else {
        buffer.writeVarInt(0); // Pose ID 0: Standing
    }

    // --- Индекс 16: Стили скина (Skin Parts) ---
    // Если этого не отправить, у игрока могут пропасть слои одежды при обновлении
    buffer.writeByte(16);     // Индекс 16 (для игрока в 1.16)
    buffer.writeVarInt(0);    // Тип 0 (Byte)
    buffer.writeByte(0x7F);   // Включаем все части скина (плащ, куртка и т.д.)

    buffer.writeByte(0xFF);   // Конец списка метадаты
}