#include "../../../../include/network/packet/play/BlockPlacePacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/game/world/World.h"
#include "../../../../include/Server.h"
#include "../../../../include/network/packet/play/BlockChangePacket.h"
#include "../../../../include/Logger.h"
#include "../../../../include/utility/MinecraftRegistry.hpp"

void BlockPlacePacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (!player) return;

    Slot heldItem = player->getInventory()[36 + player->getHeldItemSlot()];
    if (heldItem.itemId <= 0) return;

    const RegistryEntry* itemInfo = MinecraftRegistry::getByItemId(heldItem.itemId);
    if (!itemInfo || itemInfo->stateId == -1) return;

    // 1. Корректируем позицию (пакет присылает координаты блока, НА который нажали)
    Vector3 placePos = position;
    switch (face) {
        case 0: placePos.y--; break; // Bottom
        case 1: placePos.y++; break; // Top (твой случай)
        case 2: placePos.z--; break; // North
        case 3: placePos.z++; break; // South
        case 4: placePos.x--; break; // West
        case 5: placePos.x++; break; // East
        default: break; 
    }

    // 2. Используем stateId для установки и рассылки
    // В 1.13+ blockId 687 — это Note Block, а Warped Nylium имеет stateId ~15000
    int finalStateId = itemInfo->stateId;

    Server::get_instance().get_world().setBlock(placePos, finalStateId);

    // Рассылаем пакет изменения блока всем игрокам
    BlockChangePacket packet(placePos, finalStateId);
    for (const auto& p : PlayerList::getInstance().getPlayers()) {
        p->getConnection()->send_packet(packet);
    }

    LOG_DEBUG("Player " + player->getNickname() + " placed " + itemInfo->name + 
              " (State: " + std::to_string(finalStateId) + ") at " + 
              std::to_string(placePos.x) + " " + std::to_string(placePos.y) + " " + std::to_string(placePos.z));
}

void BlockPlacePacket::read(PacketBuffer& buffer) {
    hand = buffer.readVarInt();
    position = buffer.readPosition();
    face = buffer.readVarInt();
    cursorX = buffer.readFloat();
    cursorY = buffer.readFloat();
    cursorZ = buffer.readFloat();
    insideBlock = buffer.readBoolean();
}
