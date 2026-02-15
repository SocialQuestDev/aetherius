#pragma once

#include <string>
#include <vector>
#include <sol/sol.hpp>
#include "commands/GameCommand.h"

class LuaPluginManager;

class LuaGameCommand : public GameCommand {
public:
    LuaGameCommand(std::string name,
                   std::string description,
                   sol::protected_function callback,
                   LuaPluginManager& manager,
                   std::string plugin_name);

    void execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) override;
    std::string getName() const override;
    std::string getDescription() const override;

private:
    std::string name_;
    std::string description_;
    sol::protected_function callback_;
    LuaPluginManager& manager_;
    std::string plugin_name_;
};
