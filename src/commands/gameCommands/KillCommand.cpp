#include "../../../include/commands/gameCommands/KillCommand.h"
#include "../../../include/game/player/Player.h"

void KillCommand::execute(Connection& connection, const std::vector<std::string>& args) {
    connection.getPlayer()->kill();
}

std::string KillCommand::getName() const {
    return "kill";
}

std::string KillCommand::getDescription() const {
    return "Kills the player.";
}
