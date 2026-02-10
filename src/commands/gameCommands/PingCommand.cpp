#include "../../../include/commands/gameCommands/PingCommand.h"
#include "../../../include/network/packet/play/ChatMessagePacket.h"

void PingCommand::execute(Connection& connection, const std::vector<std::string>& args) {
    ChatMessagePacket pingResponse("{\"text\":\"Pong!\"}", 0, UUID());
    connection.send_packet(pingResponse);
}

std::string PingCommand::getName() const {
    return "ping";
}

std::string PingCommand::getDescription() const {
    return "Replies with Pong!";
}
