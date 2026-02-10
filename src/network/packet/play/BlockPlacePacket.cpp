#include "../../../../include/network/packet/play/BlockPlacePacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/game/world/World.h"
#include "../../../../include/Server.h"
#include "../../../../include/network/packet/play/BlockChangePacket.h"
#include "../../../../include/Logger.h"

void BlockPlacePacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        ItemStack heldItem = player->getInventory()[36 + player->getHeldItemSlot()];
        if (heldItem.itemId == 0) {
            return;
        }

        Server::get_instance().get_world().setBlock(position, heldItem.itemId);
        BlockChangePacket packet(position, heldItem.itemId);
        for (const auto& p : PlayerList::getInstance().getPlayers()) {
            p->getConnection()->send_packet(packet);
        }

        LOG_DEBUG("Player " + player->getNickname() + " placed block " + std::to_string(heldItem.itemId) + " at " + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z));
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
