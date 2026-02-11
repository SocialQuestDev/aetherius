#include "network/packet/inbound/play/GetChatMessagePacket.h"
#include "game/player/PlayerList.h"
#include "console/Logger.h"
#include "network/Connection.h"
#include "Server.h"

void GetChatMessagePacket::handle(Connection &connection) {
    auto player = connection.getPlayer();
    if (!player) return;

    LOG_INFO(player->getNickname() + ": " + message);

    if (message[0] == '/') {
        Server::get_instance().get_command_registry().executeCommand(player, message);
    }
    else {
        for (const auto& p : PlayerList::getInstance().getPlayers()) {
            p->sendChatMessage(player->getNickname() + ": " + message, ChatColor::WHITE, player->getUuid());
        }
    }
}

void GetChatMessagePacket::read(PacketBuffer &buffer) {
    message = buffer.readString();
}
