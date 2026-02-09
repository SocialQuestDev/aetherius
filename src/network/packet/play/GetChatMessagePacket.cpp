#include "../../../../include/network/packet/play/GetChatMessagePacket.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/Logger.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/network/packet/play/ChatMessagePacket.h"

void GetChatMessagePacket::handle(Connection &connection) {
    LOG_DEBUG("Received chat message: " + message);

    for (auto player: PlayerList::getInstance().getPlayers()) {
        ChatMessagePacket chatPacket("{\"text\":\"Вы упали в бездну!\", \"color\":\"red\"}", 1, connection.getPlayer()->getUuid());
        player.get()->getConnection()->send_packet(chatPacket);
    }
}

void GetChatMessagePacket::read(PacketBuffer &buffer) {
    message = buffer.readString();
}
