#include "../../../../include/network/packet/play/GetChatMessagePacket.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/console/Logger.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/network/packet/play/ChatMessagePacket.h"
#include "../../../../include/Server.h"

void GetChatMessagePacket::handle(Connection &connection) {
    LOG_DEBUG("Received chat message: " + message);

    if (message[0] == '/') {
        Server::get_instance().get_command_registry().executeCommand(&connection, message);
    }
    else {
        auto player = connection.getPlayer();
        if (!player) return;

        ChatMessagePacket chatPacket("{\"text\":\"" + player->getNickname() + ": " + message + "\"}", 0, player->getUuid());
        for (const auto& p : PlayerList::getInstance().getPlayers()) {
            p->getConnection()->send_packet(chatPacket);
        }
    }
}

void GetChatMessagePacket::read(PacketBuffer &buffer) {
    message = buffer.readString();
}
