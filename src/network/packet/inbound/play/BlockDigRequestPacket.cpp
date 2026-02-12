#include "network/packet/inbound/play/BlockDigRequestPacket.h"
#include "game/player/PlayerList.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "network/packet/outbound/play/BlockDigResponsePacket.h"
#include "network/packet/outbound/play/BlockChangePacket.h"
#include "Server.h"
#include "utils/MinecraftRegistry.hpp"

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
