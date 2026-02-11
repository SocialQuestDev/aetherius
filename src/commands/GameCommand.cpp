#include "../../include/commands/GameCommand.h"
#include "../../include/network/packet/play/ChatMessagePacket.h"

void GameCommand::execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) {
    player->sendChatMessage("Chat command placeholder.");
}

std::string GameCommand::getName() const {
    return "chat";
}

std::string GameCommand::getDescription() const {
    return "Placeholder chat command.";
}
