#include "../../../../include/network/packet/login/EncryptionResponsePacket.h"
#include "../../../../include/network/packet/login/LoginSuccessPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/Server.h"
#include "../../../../include/auth/MojangAuthHelper.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/Logger.h"
#include "../../../../include/utility/VectorUtilities.h"

void EncryptionResponsePacket::read(PacketBuffer& buffer) {
    encryptedSharedSecret = buffer.readByteArray();
    encryptedVerifyToken = buffer.readByteArray();
}

void EncryptionResponsePacket::handle(Connection& connection) {
    Server& server = Server::get_instance();
    auto serverCfg = server.get_config();

    if (!connection.is_waiting_for_encryption()) {
        LOG_ERROR("Unexpected encryption response from " + connection.get_nickname());
        return;
    }

    std::vector<uint8_t> sharedSecret = server.decrypt_rsa(encryptedSharedSecret);
    std::vector<uint8_t> decryptedToken = server.decrypt_rsa(encryptedVerifyToken);

    if (!vectors_equal(decryptedToken, connection.get_verify_token())) {
        LOG_ERROR("Verify token mismatch for " + connection.get_nickname());
        // Disconnect
        return;
    }

    connection.enable_encryption(sharedSecret);

    std::string nickname = connection.get_nickname();
    auto [uuid, textures] = auth::get_uuid(nickname);

    if (uuid.high == 0 && uuid.low == 0) {
        LOG_ERROR("Failed to authenticate player " + nickname + " with Mojang");
        return;
    }

    if (serverCfg["server"]["compression_enabled"].value_or(false)) {
        int threshold = serverCfg["server"]["compression_threshold"].value_or(256);

        PacketBuffer compPacket;
        compPacket.writeVarInt(0x03); // Set Compression
        compPacket.writeVarInt(threshold);

        connection.send_packet_raw(compPacket.finalize(false, -1, nullptr));
        connection.set_compression(true);
    }

    auto player = std::make_shared<Player>(
        PlayerList::getInstance().getNextPlayerId(),
        uuid,
        nickname,
        textures,
        connection.shared_from_this()
    );

    connection.setPlayer(player);
    PlayerList::getInstance().addPlayer(player);

    LoginSuccessPacket success(uuid, nickname);
    connection.send_packet(success);

    connection.setState(State::PLAY);
    connection.send_join_game();
    connection.start_keep_alive_timer();

    LOG_INFO("Online player logged in: " + nickname + " [" + uuid_to_string(uuid.high, uuid.low) + "]");
}