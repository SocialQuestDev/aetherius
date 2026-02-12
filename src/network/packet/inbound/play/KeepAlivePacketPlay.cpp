#include "network/packet/inbound/play/KeepAlivePacketPlay.h"
#include "network/packet/outbound/play/PlayerInfoPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "console/Logger.h"
#include <chrono>

void KeepAlivePacketPlay::handle(Connection& connection) {
    connection.handle_keep_alive(keepAliveId);

    auto player = connection.getPlayer();
    if (player) {
        PlayerInfoPacket updateLatency(PlayerInfoPacket::UPDATE_LATENCY, {player});
        for (const auto& p : PlayerList::getInstance().getPlayers()) {
            p->getConnection()->send_packet(updateLatency);
        }
    }
}

void KeepAlivePacketPlay::read(PacketBuffer& buffer) {
    keepAliveId = buffer.readLong();
}
