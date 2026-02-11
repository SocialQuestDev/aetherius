#include "../../../include/commands/gameCommands/HelpGameCommand.h"
#include "../../../include/Server.h"
#include "../../../include/game/player/Player.h"
#include "../../../include/other/ChatColor.h"
#include "../../../include/console/Logger.h"

void HelpGameCommand::execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) {
    if (player) {
        player->sendChatMessage("Available commands:", ChatColor::YELLOW);
        const auto& commands = Server::get_instance().get_command_registry().get_game_commands();
        for (const auto& [name, command] : commands) {
            player->sendChatMessage("/" + name + " - " + command->getDescription(), ChatColor::GRAY);
        }
    }
    else {
        LOG_INFO("Available commands:");
        const auto& commands = Server::get_instance().get_command_registry().get_game_commands();
        for (const auto& [name, command] : commands) {
            LOG_INFO("/" + name + " - " + command->getDescription());
        }
    }
}

std::string HelpGameCommand::getName() const {
    return "help";
}

std::string HelpGameCommand::getDescription() const {
    return "Lists all available commands.";
}
