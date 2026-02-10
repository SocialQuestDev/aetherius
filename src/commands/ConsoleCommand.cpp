#include "../../include/commands/ConsoleCommand.h"
#include "../../include/network/packet/play/ChatMessagePacket.h"

void ConsoleCommand::execute(Connection& connection, const std::vector<std::string>& args) {
    // Placeholder
    ChatMessagePacket response("{\"text\":\"Console command placeholder.\"}", 0, UUID());
    connection.send_packet(response);
}

std::string ConsoleCommand::getName() const {
    return "console";
}

std::string ConsoleCommand::getDescription() const {
    return "Placeholder console command.";
}
