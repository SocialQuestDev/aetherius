#pragma once

#include "commands/GameCommand.h"
#include <string>
#include <vector>

class TimeCommand : public GameCommand {
public:
    std::string getName() const override;
    std::string getDescription() const override;
    void execute(std::shared_ptr<Player> sender, const std::vector<std::string>& args) override;
};
