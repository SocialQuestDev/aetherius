#pragma once

#include <sol/sol.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <optional>
#include <functional>
#include "commands/CommandRegistry.h"
#include "plugins/IPluginManager.h"

class Player;

struct LuaPlugin {
    std::string name;
    std::string version;
    std::filesystem::path path;
    sol::table table;
    sol::protected_function on_load;
    sol::protected_function on_unload;
    sol::protected_function on_tick;
    std::vector<std::string> registered_game_commands;
    std::vector<std::pair<std::string, std::string>> registered_hooks;
};

class LuaPluginManager : public IPluginManager {
public:
    explicit LuaPluginManager(CommandRegistry& command_registry);

    void start();
    void stop();

    void load_all();
    void unload_all();
    void reload_all();
    void tick();
    void emit_server_start();
    void emit_server_stop();
    void emit_player_join(const PluginPlayer& player);
    void emit_player_leave(const PluginPlayer& player, const std::string& reason);
    bool emit_player_chat(const PluginPlayer& player, std::string& message);

    void invoke_command(sol::protected_function callback,
                        const std::string& plugin_name,
                        const std::optional<PluginPlayer>& player,
                        const std::vector<std::string>& args);

private:
    struct LuaHook {
        std::string plugin_name;
        std::string hook_name;
        sol::protected_function callback;
    };

    bool load_plugin(const std::filesystem::path& plugin_dir);
    void unload_plugin(LuaPlugin& plugin);
    void bind_api();
    void set_package_path_for_plugin(const std::filesystem::path& plugin_dir);
    void register_game_command(const std::string& name,
                               const std::string& description,
                               sol::protected_function callback);
    void register_hook(const std::string& event,
                       const std::string& name,
                       sol::protected_function callback);
    void remove_hook(const std::string& event, const std::string& name);
    bool run_lua_file(const std::filesystem::path& path, const std::string& context);
    bool resolve_plugin_path(const std::filesystem::path& plugin_dir,
                             const std::string& relative_path,
                             std::filesystem::path& out_path) const;
    void run_game_task(const std::function<void()>& task);
    void enqueue_task(std::function<void()> task);
    void run_loop();
    void call_with_plugin(LuaPlugin* plugin, const std::function<void()>& fn);

    CommandRegistry& command_registry_;
    sol::state lua_;
    std::unordered_map<std::string, LuaPlugin> plugins_;
    std::unordered_map<std::string, std::vector<LuaHook>> hooks_;
    std::filesystem::path plugins_dir_;
    LuaPlugin* current_plugin_ = nullptr;

    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::deque<std::function<void()>> task_queue_;
    std::thread worker_;
    bool running_ = false;
};
