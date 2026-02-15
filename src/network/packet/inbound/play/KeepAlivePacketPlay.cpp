#include "network/packet/inbound/play/KeepAlivePacketPlay.h"
#include "network/packet/outbound/play/PlayerInfoPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "console/Logger.h"
#include "Server.h"
#include <chrono>

void KeepAlivePacketPlay::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const uint64_t keep_id = keepAliveId;
    Server::get_instance().post_game_task([self, keep_id]() {
        self->handle_keep_alive(keep_id);

        auto player = self->getPlayer();
        if (player) {
            PlayerInfoPacket updateLatency(PlayerInfoPacket::UPDATE_LATENCY, {player});
            for (const auto& p : PlayerList::getInstance().getPlayers()) {
                p->getConnection()->send_packet(updateLatency);
            }
        }
    });
}

void KeepAlivePacketPlay::read(PacketBuffer& buffer) {
    keepAliveId = buffer.readLong();
}
