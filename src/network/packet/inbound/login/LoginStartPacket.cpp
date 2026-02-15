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
    auto self = connection.shared_from_this();
    const std::string name = nickname;
    Server::get_instance().post_game_task([self, name]() {
        LOG_INFO("New login attempt from " + self->socket().remote_endpoint().address().to_string() + " with nickname: " + name);
        self->set_nickname(name);

        Server& server = Server::get_instance();
        auto serverCfg = server.get_config();

        if (PlayerList::getInstance().getPlayer(name) != nullptr) {
            PacketBuffer alreadyJoinedDisconnect;

            alreadyJoinedDisconnect.writeVarInt(0x00);
            alreadyJoinedDisconnect.writeString("{\"text\":\"You are already connected to a server.\", \"color\":\"red\"}");

            self->send_packet(alreadyJoinedDisconnect);

            try {
                self->socket().close();
            } catch (std::runtime_error& e) {
                LOG_ERROR("Error closing socket for " + name + ": " + e.what());
            }

            return;
        }

        AuthType authType = static_cast<AuthType>(serverCfg["server"]["auth_mode"].value_or(0));
        LOG_INFO("Using " + std::string(magic_enum::enum_name(authType)) + " auth");
        std::unique_ptr<Auth> auth = Auth::Create(authType);

        if (!auth) {
            LOG_ERROR("Failed to create auth instance.");
            return;
        }

        if (authType == AuthType::Mojang) {
            std::vector<uint8_t> verifyTokenTemp = auth->generateVerifyToken();
            self->set_verify_token(verifyTokenTemp);

            PacketBuffer encReq;
            encReq.writeVarInt(0x01);
            encReq.writeString("");   // Server ID
            encReq.writeByteArray(server.get_public_key());
            encReq.writeByteArray(verifyTokenTemp);

            self->set_waiting_for_encryption_response(true);
            self->send_packet_raw(encReq.finalize(false, -1, nullptr));

            auto player = std::make_shared<Player>(PlayerList::getInstance().getNextPlayerId(), UUID(), name, "", self);
            player->setAuth(std::move(auth));
            self->setPlayer(player);

            return;
        }

        if (serverCfg["server"]["compression_enabled"].value_or(false)) {
            int compression_threshold = serverCfg["server"]["compression_threshold"].value_or(256);

            PacketBuffer compPacket;
            compPacket.writeVarInt(0x03);
            compPacket.writeVarInt(compression_threshold);

            self->send_packet_raw(compPacket.finalize(false, -1, nullptr));
            self->set_compression(true);
        }

        Auth::PlayerProfile profile = auth->getPlayerProfile(name);

        auto player = std::make_shared<Player>(PlayerList::getInstance().getNextPlayerId(), profile.uuid, name, profile.textures, self);
        player->setAuth(std::move(auth));
        self->setPlayer(player);
        PlayerList::getInstance().addPlayer(player);

        LoginSuccessPacket success(profile.uuid, name);
        self->send_packet(success);

        self->setState(State::PLAY);
        self->send_join_game();
        self->start_keep_alive_timer();

        for (const auto& client: PlayerList::getInstance().getPlayers()) {
            ChatMessagePacket packet(R"({"text":")" + name + R"( joined the game", "color":"yellow"})", 1, profile.uuid);
            client.get()->getConnection()->send_packet(packet);
        }
    });
}

void LoginStartPacket::read(PacketBuffer& buffer) {
    nickname = buffer.readString();
}
