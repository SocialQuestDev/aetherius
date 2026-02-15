#include "network/packet/inbound/play/BlockDigRequestPacket.h"
#include "game/player/PlayerList.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "network/packet/outbound/play/BlockDigResponsePacket.h"
#include "network/packet/outbound/play/BlockChangePacket.h"
#include "Server.h"
#include "utils/MinecraftRegistry.hpp"

void BlockDigRequestPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const int dig_status = status;
    const Vector3 dig_pos = position;
    Server::get_instance().post_game_task([self, dig_status, dig_pos]() {
        std::shared_ptr<Player> digger = self->getPlayer();
        if (!digger) {
            return;
        }

        char destroyStage = -2;

        switch (dig_status) {
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

        BlockDigResponsePacket packet(digger->getId(), dig_pos, destroyStage);
        BlockChangePacket blockChangePacket(dig_pos, 0);
        for (const auto& player : PlayerList::getInstance().getPlayers()) {
            if (player->getId() != digger->getId()) {
                player->getConnection()->send_packet(packet);
                player->getConnection()->send_packet(blockChangePacket);
            }
        }
        Server::get_instance().get_world().setBlock(dig_pos, 0);
    });
}

void BlockDigRequestPacket::read(PacketBuffer& buffer) {
    status = buffer.readVarInt();
    position = buffer.readPosition();
    face = buffer.readByte();
}
