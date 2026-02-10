#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Command.h"

class Connection;

class CommandRegistry {
public:
    void registerGameCommand(std::unique_ptr<Command> command);
    void registerConsoleCommand(std::unique_ptr<Command> command);
    void executeCommand(Connection* connection, const std::string& message);
    const std::unordered_map<std::string, std::unique_ptr<Command>>& get_game_commands() const;

private:
    std::unordered_map<std::string, std::unique_ptr<Command>> gameCommands;
    std::unordered_map<std::string, std::unique_ptr<Command>> consoleCommands;
};
