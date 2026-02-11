#include "../../include/commands/ConsoleCommand.h"
#include "../../include/network/packet/play/ChatMessagePacket.h"

void ConsoleCommand::execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) {
    player->sendChatMessage("Console command placeholder.");
}

std::string ConsoleCommand::getName() const {
    return "console";
}

std::string ConsoleCommand::getDescription() const {
    return "Placeholder console command.";
}
