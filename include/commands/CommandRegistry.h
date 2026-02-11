#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "commands/Command.h"
#include "commands/ConsoleCommand.h"
#include "commands/GameCommand.h"

class Connection;
class Player;

class CommandRegistry {
public:
    void registerGameCommand(std::unique_ptr<GameCommand> command);
    void registerConsoleCommand(std::unique_ptr<ConsoleCommand> command);
    void executeCommand(std::shared_ptr<Player> player, const std::string& message);

    const std::unordered_map<std::string, std::unique_ptr<Command>>& get_game_commands() const;
    const std::unordered_map<std::string, std::unique_ptr<Command>>& get_console_commands() const;

private:
    std::unordered_map<std::string, std::unique_ptr<Command>> gameCommands;
    std::unordered_map<std::string, std::unique_ptr<Command>> consoleCommands;
};
