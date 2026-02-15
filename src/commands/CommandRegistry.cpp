#include "commands/CommandRegistry.h"
#include "network/packet/outbound/play/ChatMessagePacket.h"
#include "utils/String.h"
#include "console/Logger.h"
#include "game/player/Player.h"

void CommandRegistry::registerGameCommand(std::unique_ptr<GameCommand> command) {
    gameCommands[command->getName()] = std::move(command);
}

void CommandRegistry::registerConsoleCommand(std::unique_ptr<ConsoleCommand> command) {
    consoleCommands[command->getName()] = std::move(command);
}

bool CommandRegistry::hasGameCommand(const std::string& name) const {
    return gameCommands.find(name) != gameCommands.end();
}

void CommandRegistry::unregisterGameCommand(const std::string& name) {
    gameCommands.erase(name);
}

void CommandRegistry::executeCommand(std::shared_ptr<Player> player, const std::string& message) {
    std::string command_text = message;
    if (player && !message.empty() && message[0] == '/') {
        command_text = message.substr(1);
    }

    std::vector<std::string> parts = String::split(command_text, ' ');
    if (parts.empty()) return;

    if (player) {
        auto it = gameCommands.find(parts[0]);
        if (it != gameCommands.end()) {
            std::vector<std::string> args(parts.begin() + 1, parts.end());
            it->second->execute(player, args);
        } else {
            player->sendChatMessage("Unknown command: " + message);
        }
    } else {
        auto it = consoleCommands.find(parts[0]);
        if (it != consoleCommands.end()) {
            std::vector<std::string> args(parts.begin() + 1, parts.end());
            it->second->execute(nullptr, args);
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
