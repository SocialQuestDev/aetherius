#include "../../include/commands/GameCommand.h"
#include "../../include/network/packet/play/ChatMessagePacket.h"

void GameCommand::execute(Connection& connection, const std::vector<std::string>& args) {
    // Placeholder
    ChatMessagePacket response("{\"text\":\"Chat command placeholder.\"}", 0, UUID());
    connection.send_packet(response);
}

std::string GameCommand::getName() const {
    return "chat";
}

std::string GameCommand::getDescription() const {
    return "Placeholder chat command.";
}
