#include "network/packet/inbound/login/LoginStartPacket.h"
#include "network/packet/outbound/login/LoginSuccessPacket.h"
#include "network/Connection.h"
#include "Server.h"
#include "game/player/PlayerList.h"
#include "game/player/Player.h"
#include "auth/Auth.h"
#include "console/Logger.h"
#include "network/packet/outbound/play/ChatMessagePacket.h"
#include <cstddef>
#include <stdexcept>
#include <magic_enum.hpp>

void LoginStartPacket::handle(Connection& connection) {
    LOG_INFO("New login attempt from " + connection.socket().remote_endpoint().address().to_string() + " with nickname: " + nickname);
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

    AuthType authType = static_cast<AuthType>(serverCfg["server"]["auth_mode"].value_or(0));
    LOG_INFO("Using " + std::string(magic_enum::enum_name(authType)) + " auth");
    std::unique_ptr<Auth> auth = Auth::Create(authType);

    if (!auth) {
        LOG_ERROR("Failed to create auth instance.");
        // Disconnect or handle error appropriately
        return;
    }

    if (authType == AuthType::Mojang) {
        std::vector<uint8_t> verifyTokenTemp = auth->generateVerifyToken();
        connection.set_verify_token(verifyTokenTemp);

        PacketBuffer encReq;
        encReq.writeVarInt(0x01);
        encReq.writeString("");   // Server ID
        encReq.writeByteArray(server.get_public_key());
        encReq.writeByteArray(verifyTokenTemp);

        connection.set_waiting_for_encryption_response(true);
        connection.send_packet_raw(encReq.finalize(false, -1, nullptr));

        auto player = std::make_shared<Player>(PlayerList::getInstance().getNextPlayerId(), UUID(), nickname, "", connection.shared_from_this());
        player->setAuth(std::move(auth));
        connection.setPlayer(player);

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

    Auth::PlayerProfile profile = auth->getPlayerProfile(nickname);

    auto player = std::make_shared<Player>(PlayerList::getInstance().getNextPlayerId(), profile.uuid, nickname, profile.textures, connection.shared_from_this());
    player->setAuth(std::move(auth));
    connection.setPlayer(player);
    PlayerList::getInstance().addPlayer(player);

    LoginSuccessPacket success(profile.uuid, nickname);
    connection.send_packet(success);

    connection.setState(State::PLAY);
    connection.send_join_game();
    connection.start_keep_alive_timer();

    for (const auto& client: PlayerList::getInstance().getPlayers()) {
        ChatMessagePacket packet(R"({"text":")" + nickname + R"( joined the game", "color":"yellow"})", 1, profile.uuid);
        client.get()->getConnection()->send_packet(packet);
    }
}

void LoginStartPacket::read(PacketBuffer& buffer) {
    nickname = buffer.readString();
}
