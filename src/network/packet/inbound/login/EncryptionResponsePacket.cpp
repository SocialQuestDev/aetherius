#include "network/packet/inbound/login/EncryptionResponsePacket.h"
#include "network/packet/outbound/login/LoginSuccessPacket.h"
#include "network/Connection.h"
#include "Server.h"
#include "game/player/PlayerList.h"
#include "game/player/Player.h"
#include "console/Logger.h"
#include "utils/VectorUtilities.h"
#include "network/packet/outbound/play/ChatMessagePacket.h"

void EncryptionResponsePacket::read(PacketBuffer& buffer) {
    encryptedSharedSecret = buffer.readByteArray();
    encryptedVerifyToken = buffer.readByteArray();
}

void EncryptionResponsePacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const std::vector<uint8_t> shared_secret = encryptedSharedSecret;
    const std::vector<uint8_t> verify_token = encryptedVerifyToken;
    Server::get_instance().post_game_task([self, shared_secret, verify_token]() {
        Server& server = Server::get_instance();
        auto serverCfg = server.get_config();
        auto player = self->getPlayer();

        if (!player) {
            LOG_ERROR("No player associated with connection during encryption response.");
            return;
        }

        auto& auth = player->getAuth();
        if (!auth) {
            LOG_ERROR("No auth instance for player " + player->getNickname());
            return;
        }

        if (!self->is_waiting_for_encryption_response()) {
            LOG_ERROR("Unexpected encryption response from " + player->getNickname());
            return;
        }

        std::vector<uint8_t> sharedSecret = server.decrypt_rsa(shared_secret);
        std::vector<uint8_t> decryptedToken = server.decrypt_rsa(verify_token);

        if (!vectors_equal(decryptedToken, self->get_verify_token())) {
            LOG_ERROR("Verify token mismatch for " + player->getNickname());
            return;
        }

        self->enable_encryption(sharedSecret);

        std::string nickname = player->getNickname();
        Auth::PlayerProfile profile = auth->getPlayerProfile(nickname);

        if (profile.uuid.high == 0 && profile.uuid.low == 0) {
            LOG_ERROR("Failed to authenticate player " + nickname + " with Mojang");
            return;
        }

        player->setUuid(profile.uuid);
        player->setSkin(profile.textures);
        PlayerList::getInstance().addPlayer(player);

        if (serverCfg["server"]["compression_enabled"].value_or(false)) {
            int threshold = serverCfg["server"]["compression_threshold"].value_or(256);

            PacketBuffer compPacket;
            compPacket.writeVarInt(0x03); // Set Compression
            compPacket.writeVarInt(threshold);

            self->send_packet_raw(self->finalize_packet(compPacket));
            self->set_compression(true);
        }

        LoginSuccessPacket success(profile.uuid, nickname);
        self->send_packet(success);

        self->setState(State::PLAY);
        self->send_join_game();
        self->start_keep_alive_timer();

        for (const auto& client: PlayerList::getInstance().getPlayers()) {
            client->sendChatMessage(nickname + " joined the game", ChatColor::YELLOW);
        }

        LOG_INFO("Online player logged in: " + nickname + " [" + uuid_to_string(profile.uuid.high, profile.uuid.low) + "]");
    });
}
