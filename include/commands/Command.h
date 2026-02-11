#pragma once

#include <string>
#include <vector>
#include <functional>
#include "network/Connection.h"

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) = 0;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
};
