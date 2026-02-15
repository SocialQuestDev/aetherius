#pragma once

#include <string>

struct PluginPlayer {
    int id = 0;
    std::string name;
};

class IPluginManager {
public:
    virtual ~IPluginManager() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void load_all() = 0;
    virtual void unload_all() = 0;
    virtual void reload_all() = 0;
    virtual void tick() = 0;

    virtual void emit_server_start() = 0;
    virtual void emit_server_stop() = 0;
    virtual void emit_player_join(const PluginPlayer& player) = 0;
    virtual void emit_player_leave(const PluginPlayer& player, const std::string& reason) = 0;
    virtual bool emit_player_chat(const PluginPlayer& player, std::string& message) = 0;
};
