#include "../../../../include/network/packet/play/BlockDigRequestPacket.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/network/packet/play/BlockDigResponsePacket.h"
#include "../../../../include/network/packet/play/BlockChangePacket.h"
#include "../../../../include/Server.h"
#include "../../../../include/utility/MinecraftRegistry.hpp"

void BlockDigRequestPacket::handle(Connection& connection) {
    std::shared_ptr<Player> digger = connection.getPlayer();
    if (!digger) {
        return;
    }

    char destroyStage = -2;

    switch (status) {
        case 0:
            destroyStage = 0;
            break;
        case 1:
            destroyStage = -1;
            break;
        case 2:
            destroyStage = 1;
            break;
    }

    BlockDigResponsePacket packet(digger->getId(), this->position, destroyStage);
    BlockChangePacket blockChangePacket(this->position, 0);
    for (const auto& player : PlayerList::getInstance().getPlayers()) {
        if (player->getId() != digger->getId()) {
            player->getConnection()->send_packet(packet);
            player->getConnection()->send_packet(blockChangePacket);
        }
    }
    Server::get_instance().get_world().setBlock(this->position, 0);
}

void BlockDigRequestPacket::read(PacketBuffer& buffer) {
    status = buffer.readVarInt();
    position = buffer.readPosition();
    face = buffer.readByte();
}
