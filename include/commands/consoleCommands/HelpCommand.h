#pragma once

#include "commands/ConsoleCommand.h"

class HelpCommand : public ConsoleCommand {
public:
    void execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) override;
    std::string getName() const override;
    std::string getDescription() const override;
};
