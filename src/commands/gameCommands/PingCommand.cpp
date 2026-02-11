#include "commands/gameCommands/PingCommand.h"
#include "network/packet/outbound/play/ChatMessagePacket.h"
#include "console/Logger.h"
#include "game/player/Player.h"

void PingCommand::execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) {
    if (player) {
        player->sendChatMessage("Pong!");
    } else {
        LOG_INFO("Pong!");
    }
}

std::string PingCommand::getName() const {
    return "ping";
}

std::string PingCommand::getDescription() const {
    return "Replies with Pong!";
}
