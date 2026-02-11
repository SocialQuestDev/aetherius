#pragma once

#include "commands/GameCommand.h"

class KillCommand : public GameCommand {
public:
    void execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) override;
    std::string getName() const override;
    std::string getDescription() const override;
};
