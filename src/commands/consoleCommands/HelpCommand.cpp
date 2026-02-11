#include "commands/consoleCommands/HelpCommand.h"
#include "Server.h"
#include "console/Logger.h"

void HelpCommand::execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) {
    LOG_INFO("Available console commands:");
    const auto& commands = Server::get_instance().get_command_registry().get_console_commands();
    for (const auto& [name, command] : commands) {
        LOG_INFO(name + " - " + command->getDescription());
    }
}

std::string HelpCommand::getName() const {
    return "help";
}

std::string HelpCommand::getDescription() const {
    return "Lists all available console commands.";
}
