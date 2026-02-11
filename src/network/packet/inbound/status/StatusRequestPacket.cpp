#include "network/packet/inbound/status/StatusRequestPacket.h"
#include "network/packet/outbound/status/StatusResponsePacket.h"
#include "network/Connection.h"
#include "Server.h"
#include "game/player/PlayerList.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void StatusRequestPacket::handle(Connection& connection) {
    json resp;
    auto config = Server::get_instance().get_config();
    PlayerList& list = PlayerList::getInstance();

    resp["version"]["name"] = "1.16.5";
    resp["version"]["protocol"] = 754;

    resp["players"]["max"] = config["server"]["max_player"].value_or(20);
    resp["players"]["online"] = list.getPlayers().size();

    json playerSample = json::array();

    for (const auto& player: list.getPlayers()) {
        UUID uuid = player->getUuid();

        playerSample.push_back({
            {"name", player->getNickname()},
            {"id", uuid_to_string(uuid.high, uuid.low)}
        });
    }

    resp["players"]["sample"] = playerSample;
    resp["description"]["text"] = Server::get_instance().get_config()["server"]["motd"].value_or("An Aetherius Server");

    //std::string icon = getIconBase64(config["server"]["icon_path"].value_or("server-icon.png"));
    //if (!icon.empty()) {
    //    resp["favicon"] = icon;
    //}

    StatusResponsePacket response(resp.dump());
    connection.send_packet(response);
}

void StatusRequestPacket::read(PacketBuffer &buffer) {
    // Nothing to read
}
