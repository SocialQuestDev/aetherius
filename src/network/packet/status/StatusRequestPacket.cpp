#include "../../../../include/network/packet/status/StatusRequestPacket.h"
#include "../../../../include/network/packet/status/StatusResponsePacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/Server.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void StatusRequestPacket::handle(Connection& connection) {
    json resp;
    resp["version"] = {{"name", "1.16.5"}, {"protocol", 754}};
    resp["players"] = {{"max", 20}, {"online", 0}, {"sample", json::array()}};
    resp["description"]["text"] = "Aetherius Server";

    StatusResponsePacket response(resp.dump());
    connection.send_packet(response);
}

void StatusRequestPacket::read(PacketBuffer& buffer) {
    // No fields to read
}
