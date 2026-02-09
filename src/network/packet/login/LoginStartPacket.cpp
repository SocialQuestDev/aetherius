#include "../../../../include/network/packet/login/LoginStartPacket.h"
#include "../../../../include/network/packet/login/LoginSuccessPacket.h"
#include "../../../../include/network/packet/login/SetCompressionPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/Server.h"
#include "../../../../include/utility/VectorUtilities.h"
#include "../../../../include/game/player/PlayerList.h"
#include "../../../../include/game/player/Player.h"

void LoginStartPacket::handle(Connection& connection) {
    Server& server = Server::get_instance();

    auto player = std::make_shared<Player>(PlayerList::getInstance().getNextPlayerId(), nickname, connection.shared_from_this());
    connection.setPlayer(player);
    PlayerList::getInstance().addPlayer(player);

    if (server.get_config()["server"]["compression_enabled"].value_or(false)) {
        int th = server.get_config()["server"]["compression_threshold"].value_or(256);
        SetCompressionPacket compressionPacket(th);

        PacketBuffer buffer;
        buffer.writeVarInt(compressionPacket.getPacketId());
        compressionPacket.write(buffer);
        connection.send_packet_raw(buffer.finalize(false, -1, nullptr));

        connection.set_compression(true);
    }

    LoginSuccessPacket success(get_offline_UUID_128(nickname), nickname);
    connection.send_packet(success);

    connection.setState(State::PLAY);
    connection.send_join_game();
    connection.start_keep_alive_timer();
}

void LoginStartPacket::read(PacketBuffer& buffer) {
    nickname = buffer.readString();
}
