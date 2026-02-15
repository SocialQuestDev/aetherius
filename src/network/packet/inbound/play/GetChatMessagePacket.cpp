#include "network/packet/inbound/play/GetChatMessagePacket.h"
#include "game/player/PlayerList.h"
#include "console/Logger.h"
#include "network/Connection.h"
#include "Server.h"

void GetChatMessagePacket::handle(Connection &connection) {
    auto self = connection.shared_from_this();
    const std::string msg = message;
    Server::get_instance().post_game_task([self, msg]() {
        auto player = self->getPlayer();
        if (!player) return;

        LOG_INFO(player->getNickname() + ": " + msg);

        if (!msg.empty() && msg[0] == '/') {
            Server::get_instance().get_command_registry().executeCommand(player, msg);
        }
        else {
            for (const auto& p : PlayerList::getInstance().getPlayers()) {
                p->sendChatMessage(player->getNickname() + ": " + msg, ChatColor::WHITE, player->getUuid());
            }
        }
    });
}

void GetChatMessagePacket::read(PacketBuffer &buffer) {
    message = buffer.readString();
}
