#include "../../include/commands/CommandRegistry.h"
#include "../../include/network/packet/play/ChatMessagePacket.h"
#include "../../include/utility/String.h"
#include "../../include/console/Logger.h"
#include "../../include/network/Connection.h"

void CommandRegistry::registerGameCommand(std::unique_ptr<Command> command) {
    gameCommands[command->getName()] = std::move(command);
}

void CommandRegistry::registerConsoleCommand(std::unique_ptr<Command> command) {
    consoleCommands[command->getName()] = std::move(command);
}

void CommandRegistry::executeCommand(Connection* connection, const std::string& message) {
    std::vector<std::string> parts = String::split(message.substr(1), ' ');
    if (parts.empty()) return;

    std::unordered_map<std::string, std::unique_ptr<Command>>* commands;
    if (connection) {
        commands = &gameCommands;
    } else {
        commands = &consoleCommands;
    }

    auto it = commands->find(parts[0]);
    if (it != commands->end()) {
        std::vector<std::string> args(parts.begin() + 1, parts.end());
        it->second->execute(*connection, args);
    } else {
        if (connection) {
            ChatMessagePacket unknownCommand("{\"text\":\"Unknown command: " + message + "\"}", 0, UUID());
            connection->send_packet(unknownCommand);
        } else {
            LOG_INFO("Unknown command: " + message);
        }
    }
}

const std::unordered_map<std::string, std::unique_ptr<Command>>& CommandRegistry::get_game_commands() const {
    return gameCommands;
}
