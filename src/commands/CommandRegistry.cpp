#include "../../include/commands/CommandRegistry.h"
#include "../../include/network/packet/play/ChatMessagePacket.h"
#include "../../include/utility/String.h"
#include "../../include/console/Logger.h"
#include "../../include/game/player/Player.h"

void CommandRegistry::registerGameCommand(std::unique_ptr<GameCommand> command) {
    gameCommands[command->getName()] = std::move(command);
}

void CommandRegistry::registerConsoleCommand(std::unique_ptr<ConsoleCommand> command) {
    consoleCommands[command->getName()] = std::move(command);
}

void CommandRegistry::executeCommand(std::shared_ptr<Player> player, const std::string& message) {
    bool isGameCommand = player || (!message.empty() && message[0] == '/');
    std::string command_text = message;
    if (isGameCommand && !message.empty() && message[0] == '/') {
        command_text = message.substr(1);
    }

    std::vector<std::string> parts = String::split(command_text, ' ');
    if (parts.empty()) return;

    std::unordered_map<std::string, std::unique_ptr<Command>>* commands;
    if (isGameCommand) {
        commands = &gameCommands;
    } else {
        commands = &consoleCommands;
    }

    auto it = commands->find(parts[0]);
    if (it != commands->end()) {
        std::vector<std::string> args(parts.begin() + 1, parts.end());
        it->second->execute(player, args);
    } else {
        if (player && player->getConnection()) {
            player->sendChatMessage("Unknown command: " + message);
        } else {
            LOG_INFO("Unknown command: " + message);
        }
    }
}

const std::unordered_map<std::string, std::unique_ptr<Command>>& CommandRegistry::get_game_commands() const {
    return gameCommands;
}

const std::unordered_map<std::string, std::unique_ptr<Command>>& CommandRegistry::get_console_commands() const {
    return consoleCommands;
}
