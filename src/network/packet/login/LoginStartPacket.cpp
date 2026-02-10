#include "../../../../include/network/packet/login/LoginStartPacket.h"
#include "../../../../include/network/packet/login/LoginSuccessPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/Server.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/auth/MojangAuthHelper.h"
#include "../../../../include/Logger.h"
#include "../../../../include/network/packet/play/ChatMessagePacket.h"
#include <cstddef>
#include <stdexcept>

void LoginStartPacket::handle(Connection& connection) {
    LOG_INFO("Player logging in: " + nickname);
    connection.set_nickname(nickname);

    Server& server = Server::get_instance();
    auto serverCfg = server.get_config();

    if (PlayerList::getInstance().getPlayer(nickname) != nullptr) {
        PacketBuffer alreadyJoinedDisconnect;
        
        alreadyJoinedDisconnect.writeVarInt(0x00);
        alreadyJoinedDisconnect.writeString("{\"text\":\"You are already connected to a server.\", \"color\":\"red\"}");

        connection.send_packet( alreadyJoinedDisconnect);

        try {
            connection.socket().close();
        } catch (std::runtime_error& e) {
            LOG_ERROR("Error closing socket for " + nickname + ": " + e.what());
        }

        return;
    }

    if (serverCfg["server"]["online_mode"].value_or(false)) {
        std::vector<uint8_t> verifyTokenTemp = auth::generate_verify_token();
        connection.set_verify_token(verifyTokenTemp);

        PacketBuffer encReq;
        encReq.writeVarInt(0x01);
        encReq.writeString("");   // Server ID
        encReq.writeByteArray(server.get_public_key());
        encReq.writeByteArray(verifyTokenTemp);

        connection.set_waiting_for_encryption(true);
        connection.send_packet_raw(encReq.finalize(false, -1, nullptr));

        return;
    }

    if (serverCfg["server"]["compression_enabled"].value_or(false)) {
        int compression_threshold = serverCfg["server"]["compression_threshold"].value_or(256);

        PacketBuffer compPacket;
        compPacket.writeVarInt(0x03);
        compPacket.writeVarInt(compression_threshold);

        connection.send_packet_raw(compPacket.finalize(false, -1, nullptr));
        connection.set_compression(true);
    }

    UUID uuid = get_offline_UUID_128(nickname);

    auto player = std::make_shared<Player>(PlayerList::getInstance().getNextPlayerId(), uuid, nickname, "", connection.shared_from_this());
    connection.setPlayer(player);
    PlayerList::getInstance().addPlayer(player);

    LoginSuccessPacket success(uuid, nickname);
    connection.send_packet(success);

    connection.setState(State::PLAY);
    connection.send_join_game();
    connection.start_keep_alive_timer();

    for (const auto& client: PlayerList::getInstance().getPlayers()) {
        ChatMessagePacket packet(R"({"text":")" + nickname + R"( joined the game", "color":"yellow"})", 1, uuid);
        client.get()->getConnection()->send_packet(packet);
    }
}

void LoginStartPacket::read(PacketBuffer& buffer) {
    nickname = buffer.readString();
}