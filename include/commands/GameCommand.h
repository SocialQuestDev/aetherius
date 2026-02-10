#pragma once

#include "Command.h"

class GameCommand : public Command {
public:
    void execute(Connection& connection, const std::vector<std::string>& args) override;
    std::string getName() const override;
    std::string getDescription() const override;
};
