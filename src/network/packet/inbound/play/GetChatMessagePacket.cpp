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

        std::string filtered_msg = msg;
        PluginPlayer snapshot{player->getId(), player->getNickname()};
        if (!Server::get_instance().get_plugin_manager().emit_player_chat(snapshot, filtered_msg)) {
            return;
        }

        LOG_INFO(player->getNickname() + ": " + filtered_msg);

        if (!filtered_msg.empty() && filtered_msg[0] == '/') {
            Server::get_instance().get_command_registry().executeCommand(player, filtered_msg);
        }
        else {
            for (const auto& p : PlayerList::getInstance().getPlayers()) {
                p->sendChatMessage(player->getNickname() + ": " + filtered_msg, ChatColor::WHITE, player->getUuid());
            }
        }
    });
}

void GetChatMessagePacket::read(PacketBuffer &buffer) {
    message = buffer.readString();
}
