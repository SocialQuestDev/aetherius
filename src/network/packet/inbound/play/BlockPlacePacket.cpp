#include "network/packet/inbound/play/BlockPlacePacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "game/world/World.h"
#include "Server.h"
#include "network/packet/outbound/play/BlockChangePacket.h"
#include "console/Logger.h"
#include "utils/MinecraftRegistry.hpp"
void BlockPlacePacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (!player) return;

    const auto& inventory = player->getInventory();
    int inventoryIndex = 36 + player->getHeldItemSlot();

    if (inventoryIndex >= inventory.size()) return;

    Slot heldItem = inventory[inventoryIndex];
    if (heldItem.itemId <= 0) return;

    const RegistryEntry* itemInfo = MinecraftRegistry::getByItemId(heldItem.itemId);
    if (!itemInfo || itemInfo->stateId == -1) return;

    Vector3 placePos = position;
    switch (face) {
        case 0: placePos.y--; break;
        case 1: placePos.y++; break;
        case 2: placePos.z--; break;
        case 3: placePos.z++; break;
        case 4: placePos.x--; break;
        case 5: placePos.x++; break;
        default: break; 
    }

    double minX = (double)placePos.x;
    double minY = (double)placePos.y;
    double minZ = (double)placePos.z;
    double maxX = minX + 1.0;
    double maxY = minY + 1.0;
    double maxZ = minZ + 1.0;

    for (const auto& p : PlayerList::getInstance().getPlayers()) {
        Vector3 pPos = p->getPosition();
        double pMinX = pPos.x - 0.3;
        double pMaxX = pPos.x + 0.3;
        double pMinY = pPos.y;
        double pMaxY = pPos.y + 1.8;
        double pMinZ = pPos.z - 0.3;
        double pMaxZ = pPos.z + 0.3;

        bool intersects = (minX < pMaxX && maxX > pMinX) &&
                          (minY < pMaxY && maxY > pMinY) &&
                          (minZ < pMaxZ && maxZ > pMinZ);

        if (intersects) {
            LOG_DEBUG("Cannot place block: Player " + p->getNickname() + " is in the way!");
            
            int oldStateId = Server::get_instance().get_world().getBlock(placePos);
            BlockChangePacket revertPacket(placePos, oldStateId);
            connection.send_packet(revertPacket);
            return; 
        }
    }

    int finalStateId = itemInfo->stateId;
    Server::get_instance().get_world().setBlock(placePos, finalStateId);

    BlockChangePacket packet(placePos, finalStateId);
    for (const auto& p : PlayerList::getInstance().getPlayers()) {
        p->getConnection()->send_packet(packet);
    }
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
