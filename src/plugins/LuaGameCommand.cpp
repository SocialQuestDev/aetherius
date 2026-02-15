#include "plugins/LuaGameCommand.h"
#include "plugins/LuaPluginManager.h"
#include "console/Logger.h"

LuaGameCommand::LuaGameCommand(std::string name,
                               std::string description,
                               sol::protected_function callback,
                               LuaPluginManager& manager,
                               std::string plugin_name)
    : name_(std::move(name)),
      description_(std::move(description)),
      callback_(std::move(callback)),
      manager_(manager),
      plugin_name_(std::move(plugin_name)) {}

void LuaGameCommand::execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) {
    if (!callback_.valid()) {
        LOG_ERROR("Lua command callback is invalid for command: " + name_);
        return;
    }

    std::optional<PluginPlayer> ref;
    if (player) {
        ref = PluginPlayer{player->getId(), player->getNickname()};
    }
    manager_.invoke_command(callback_, plugin_name_, ref, args);
}

std::string LuaGameCommand::getName() const {
    return name_;
}

std::string LuaGameCommand::getDescription() const {
    return description_;
}
