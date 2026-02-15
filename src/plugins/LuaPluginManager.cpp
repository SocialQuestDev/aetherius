#include "plugins/LuaPluginManager.h"
#include "plugins/LuaGameCommand.h"
#include "console/Logger.h"
#include "Server.h"
#include "game/player/PlayerList.h"
#include <algorithm>
#include <future>

namespace {
std::string default_plugin_name(const std::filesystem::path& path) {
    return path.filename().string();
}
}

LuaPluginManager::LuaPluginManager(CommandRegistry& command_registry)
    : command_registry_(command_registry),
      plugins_dir_(std::filesystem::current_path() / "plugins") {
    lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                        sol::lib::table, sol::lib::string, sol::lib::utf8);

    bind_api();
}

void LuaPluginManager::start() {
    if (running_) {
        return;
    }
    running_ = true;
    worker_ = std::thread([this]() { run_loop(); });
}

void LuaPluginManager::stop() {
    if (!running_) {
        return;
    }
    enqueue_task([this]() { running_ = false; });
    queue_cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void LuaPluginManager::load_all() {
    enqueue_task([this]() {
        if (!std::filesystem::exists(plugins_dir_)) {
            std::filesystem::create_directories(plugins_dir_);
        }

        std::vector<std::filesystem::path> plugin_dirs;
        for (const auto& entry : std::filesystem::directory_iterator(plugins_dir_)) {
            if (!entry.is_directory()) {
                continue;
            }
            const auto init_path = entry.path() / "init.lua";
            if (std::filesystem::exists(init_path)) {
                plugin_dirs.push_back(entry.path());
            }
        }

        std::sort(plugin_dirs.begin(), plugin_dirs.end());
        for (const auto& plugin_dir : plugin_dirs) {
            load_plugin(plugin_dir);
        }
    });
}

void LuaPluginManager::unload_all() {
    enqueue_task([this]() {
        for (auto& [name, plugin] : plugins_) {
            unload_plugin(plugin);
        }
        plugins_.clear();
        hooks_.clear();
    });
}

void LuaPluginManager::reload_all() {
    enqueue_task([this]() {
        for (auto& [name, plugin] : plugins_) {
            unload_plugin(plugin);
        }
        plugins_.clear();
        hooks_.clear();

        if (!std::filesystem::exists(plugins_dir_)) {
            std::filesystem::create_directories(plugins_dir_);
        }

        std::vector<std::filesystem::path> plugin_dirs;
        for (const auto& entry : std::filesystem::directory_iterator(plugins_dir_)) {
            if (!entry.is_directory()) {
                continue;
            }
            const auto init_path = entry.path() / "init.lua";
            if (std::filesystem::exists(init_path)) {
                plugin_dirs.push_back(entry.path());
            }
        }

        std::sort(plugin_dirs.begin(), plugin_dirs.end());
        for (const auto& plugin_dir : plugin_dirs) {
            load_plugin(plugin_dir);
        }
    });
}

void LuaPluginManager::tick() {
    enqueue_task([this]() {
        for (auto& [name, plugin] : plugins_) {
            if (plugin.on_tick.valid()) {
                call_with_plugin(&plugin, [&]() {
                    sol::protected_function_result result = plugin.on_tick();
                    if (!result.valid()) {
                        sol::error err = result;
                        LOG_ERROR("Lua on_tick error in '" + name + "': " + std::string(err.what()));
                    }
                });
            }
        }

        auto it = hooks_.find("tick");
        if (it == hooks_.end()) {
            return;
        }

        for (auto& hook : it->second) {
            auto plugin_it = plugins_.find(hook.plugin_name);
            LuaPlugin* plugin = plugin_it != plugins_.end() ? &plugin_it->second : nullptr;
            call_with_plugin(plugin, [&]() {
                sol::protected_function_result result = hook.callback();
                if (!result.valid()) {
                    sol::error err = result;
                    LOG_ERROR("Lua hook 'tick' error in '" + hook.plugin_name + ":" + hook.hook_name + "': " + std::string(err.what()));
                }
            });
        }
    });
}

void LuaPluginManager::emit_server_start() {
    enqueue_task([this]() {
        auto it = hooks_.find("server_start");
        if (it == hooks_.end()) {
            return;
        }

        for (auto& hook : it->second) {
            auto plugin_it = plugins_.find(hook.plugin_name);
            LuaPlugin* plugin = plugin_it != plugins_.end() ? &plugin_it->second : nullptr;
            call_with_plugin(plugin, [&]() {
                sol::protected_function_result result = hook.callback();
                if (!result.valid()) {
                    sol::error err = result;
                    LOG_ERROR("Lua hook 'server_start' error in '" + hook.plugin_name + ":" + hook.hook_name + "': " + std::string(err.what()));
                }
            });
        }
    });
}

void LuaPluginManager::emit_server_stop() {
    enqueue_task([this]() {
        auto it = hooks_.find("server_stop");
        if (it == hooks_.end()) {
            return;
        }

        for (auto& hook : it->second) {
            auto plugin_it = plugins_.find(hook.plugin_name);
            LuaPlugin* plugin = plugin_it != plugins_.end() ? &plugin_it->second : nullptr;
            call_with_plugin(plugin, [&]() {
                sol::protected_function_result result = hook.callback();
                if (!result.valid()) {
                    sol::error err = result;
                    LOG_ERROR("Lua hook 'server_stop' error in '" + hook.plugin_name + ":" + hook.hook_name + "': " + std::string(err.what()));
                }
            });
        }
    });
}

void LuaPluginManager::emit_player_join(const PluginPlayer& player) {
    enqueue_task([this, player]() {
        auto it = hooks_.find("player_join");
        if (it == hooks_.end()) {
            return;
        }

        for (auto& hook : it->second) {
            auto plugin_it = plugins_.find(hook.plugin_name);
            LuaPlugin* plugin = plugin_it != plugins_.end() ? &plugin_it->second : nullptr;
            call_with_plugin(plugin, [&]() {
                sol::protected_function_result result = hook.callback(player);
                if (!result.valid()) {
                    sol::error err = result;
                    LOG_ERROR("Lua hook 'player_join' error in '" + hook.plugin_name + ":" + hook.hook_name + "': " + std::string(err.what()));
                }
            });
        }
    });
}

void LuaPluginManager::emit_player_leave(const PluginPlayer& player, const std::string& reason) {
    enqueue_task([this, player, reason]() {
        auto it = hooks_.find("player_leave");
        if (it == hooks_.end()) {
            return;
        }

        for (auto& hook : it->second) {
            auto plugin_it = plugins_.find(hook.plugin_name);
            LuaPlugin* plugin = plugin_it != plugins_.end() ? &plugin_it->second : nullptr;
            call_with_plugin(plugin, [&]() {
                sol::protected_function_result result = hook.callback(player, reason);
                if (!result.valid()) {
                    sol::error err = result;
                    LOG_ERROR("Lua hook 'player_leave' error in '" + hook.plugin_name + ":" + hook.hook_name + "': " + std::string(err.what()));
                }
            });
        }
    });
}

bool LuaPluginManager::emit_player_chat(const PluginPlayer& player, std::string& message) {
    if (!running_) {
        return true;
    }

    if (std::this_thread::get_id() == worker_.get_id()) {
        auto it = hooks_.find("player_chat");
        if (it == hooks_.end()) {
            return true;
        }

        for (auto& hook : it->second) {
            auto plugin_it = plugins_.find(hook.plugin_name);
            LuaPlugin* plugin = plugin_it != plugins_.end() ? &plugin_it->second : nullptr;
            call_with_plugin(plugin, [&]() {
                sol::protected_function_result result = hook.callback(player, message);
                if (!result.valid()) {
                    sol::error err = result;
                    LOG_ERROR("Lua hook 'player_chat' error in '" + hook.plugin_name + ":" + hook.hook_name + "': " + std::string(err.what()));
                    return;
                }

                const sol::type result_type = result.get_type();
                if (result_type == sol::type::boolean) {
                    if (!result.get<bool>()) {
                        message.clear();
                    }
                } else if (result_type == sol::type::string) {
                    message = result.get<std::string>();
                }
            });

            if (message.empty()) {
                return false;
            }
        }

        return true;
    }

    auto done = std::make_shared<std::promise<std::optional<std::string>>>();
    auto future = done->get_future();
    std::string message_copy = message;

    enqueue_task([this, player, message_copy, done]() mutable {
        auto it = hooks_.find("player_chat");
        if (it != hooks_.end()) {
            std::string local_message = message_copy;
            for (auto& hook : it->second) {
                auto plugin_it = plugins_.find(hook.plugin_name);
                LuaPlugin* plugin = plugin_it != plugins_.end() ? &plugin_it->second : nullptr;
                call_with_plugin(plugin, [&]() {
                    sol::protected_function_result result = hook.callback(player, local_message);
                    if (!result.valid()) {
                        sol::error err = result;
                        LOG_ERROR("Lua hook 'player_chat' error in '" + hook.plugin_name + ":" + hook.hook_name + "': " + std::string(err.what()));
                        return;
                    }

                    const sol::type result_type = result.get_type();
                    if (result_type == sol::type::boolean) {
                        if (!result.get<bool>()) {
                            local_message.clear();
                        }
                    } else if (result_type == sol::type::string) {
                        local_message = result.get<std::string>();
                    }
                });

                if (local_message.empty()) {
                    done->set_value(std::nullopt);
                    return;
                }
            }
            message_copy = local_message;
        }
        done->set_value(message_copy);
    });

    auto result = future.get();
    if (!result.has_value()) {
        return false;
    }
    message = *result;
    return true;
}

void LuaPluginManager::invoke_command(sol::protected_function callback,
                                      const std::string& plugin_name,
                                      const std::optional<PluginPlayer>& player,
                                      const std::vector<std::string>& args) {
    if (!running_) {
        return;
    }

    auto done = std::make_shared<std::promise<void>>();
    auto future = done->get_future();

    enqueue_task([this, callback = std::move(callback), plugin_name, player, args, done]() mutable {
        LuaPlugin* plugin = nullptr;
        auto it = plugins_.find(plugin_name);
        if (it != plugins_.end()) {
            plugin = &it->second;
        }
        call_with_plugin(plugin, [&]() {
            sol::protected_function_result result;
            if (player.has_value()) {
                result = callback(player.value(), sol::as_table(args));
            } else {
                result = callback(sol::nil, sol::as_table(args));
            }
            if (!result.valid()) {
                sol::error err = result;
                LOG_ERROR("Lua command error: " + std::string(err.what()));
            }
        });
        done->set_value();
    });

    future.get();
}

bool LuaPluginManager::load_plugin(const std::filesystem::path& plugin_dir) {
    const auto init_path = plugin_dir / "init.lua";
    if (!std::filesystem::exists(init_path)) {
        LOG_WARN("Lua plugin missing init.lua: " + plugin_dir.string());
        return false;
    }

    LuaPlugin plugin;
    plugin.path = plugin_dir;
    plugin.name = default_plugin_name(plugin_dir);

    current_plugin_ = &plugin;
    set_package_path_for_plugin(plugin.path);

    sol::load_result chunk = lua_.load_file(init_path.string());
    if (!chunk.valid()) {
        sol::error err = chunk;
        LOG_ERROR("Lua load error in " + init_path.string() + ": " + std::string(err.what()));
        current_plugin_ = nullptr;
        return false;
    }

    sol::protected_function plugin_func = chunk;
    sol::protected_function_result result = plugin_func();
    if (!result.valid()) {
        sol::error err = result;
        LOG_ERROR("Lua runtime error in " + init_path.string() + ": " + std::string(err.what()));
        current_plugin_ = nullptr;
        return false;
    }

    if (result.get_type() != sol::type::table) {
        LOG_WARN("Lua plugin did not return a table: " + init_path.string());
        current_plugin_ = nullptr;
        return false;
    }

    sol::table plugin_table = result;
    plugin.table = plugin_table;
    const std::string previous_name = plugin.name;
    plugin.name = plugin_table.get_or("name", plugin.name);
    plugin.version = plugin_table.get_or("version", std::string("0.0.0"));
    plugin.on_load = plugin_table["on_load"];
    plugin.on_unload = plugin_table["on_unload"];
    plugin.on_tick = plugin_table["on_tick"];

    if (plugin.name != previous_name) {
        for (auto& [event, hooks] : hooks_) {
            for (auto& hook : hooks) {
                if (hook.plugin_name == previous_name) {
                    hook.plugin_name = plugin.name;
                }
            }
        }
    }

    if (plugins_.find(plugin.name) != plugins_.end()) {
        LOG_WARN("Lua plugin name already loaded, skipping: " + plugin.name);
        for (const auto& command_name : plugin.registered_game_commands) {
            run_game_task([this, command_name]() { command_registry_.unregisterGameCommand(command_name); });
        }
        for (const auto& hook : plugin.registered_hooks) {
            remove_hook(hook.first, hook.second);
        }
        current_plugin_ = nullptr;
        return false;
    }

    LOG_INFO("Loading Lua plugin: " + plugin.name + " v" + plugin.version);
    if (plugin.on_load.valid()) {
        call_with_plugin(&plugin, [&]() {
            sol::protected_function_result load_result = plugin.on_load();
            if (!load_result.valid()) {
                sol::error err = load_result;
                LOG_ERROR("Lua on_load error in '" + plugin.name + "': " + std::string(err.what()));
            }
        });
    }
    current_plugin_ = nullptr;

    plugins_.emplace(plugin.name, std::move(plugin));
    return true;
}

void LuaPluginManager::unload_plugin(LuaPlugin& plugin) {
    LOG_INFO("Unloading Lua plugin: " + plugin.name);

    if (plugin.on_unload.valid()) {
        call_with_plugin(&plugin, [&]() {
            sol::protected_function_result result = plugin.on_unload();
            if (!result.valid()) {
                sol::error err = result;
                LOG_ERROR("Lua on_unload error in '" + plugin.name + "': " + std::string(err.what()));
            }
        });
    }

    for (const auto& command_name : plugin.registered_game_commands) {
        run_game_task([this, command_name]() { command_registry_.unregisterGameCommand(command_name); });
    }

    for (const auto& hook : plugin.registered_hooks) {
        remove_hook(hook.first, hook.second);
    }
}

void LuaPluginManager::bind_api() {
    lua_.new_usertype<PluginPlayer>("Player",
        "get_name", [](const PluginPlayer& player) { return player.name; },
        "send_message", [](const PluginPlayer& player, const std::string& message) {
            const int player_id = player.id;
            Server::get_instance().post_game_task([player_id, message]() {
                auto target = PlayerList::getInstance().getPlayer(player_id);
                if (target) {
                    target->sendChatMessage(message);
                }
            });
        }
    );

    sol::table log = lua_.create_table();
    log.set_function("info", [](const std::string& message) {
        LOG_INFO("[Lua] " + message);
    });
    log.set_function("debug", [](const std::string& message) {
        LOG_DEBUG("[Lua] " + message);
    });
    log.set_function("warn", [](const std::string& message) {
        LOG_WARN("[Lua] " + message);
    });
    log.set_function("error", [](const std::string& message) {
        LOG_ERROR("[Lua] " + message);
    });

    sol::table api = lua_.create_table();
    api["log"] = log;
    api.set_function("register_command",
        [this](const std::string& name, const std::string& description, sol::protected_function callback) {
            register_game_command(name, description, std::move(callback));
        });
    api.set_function("hook_add",
        [this](const std::string& event, const std::string& name, sol::protected_function callback) {
            register_hook(event, name, std::move(callback));
        });
    api.set_function("hook_remove",
        [this](const std::string& event, const std::string& name) {
            remove_hook(event, name);
        });
    api.set_function("include",
        [this](const std::string& relative_path) {
            if (!current_plugin_) {
                LOG_WARN("Lua include called outside plugin context");
                return;
            }
            std::filesystem::path resolved;
            if (!resolve_plugin_path(current_plugin_->path, relative_path, resolved)) {
                LOG_WARN("Lua include blocked: " + relative_path);
                return;
            }
            set_package_path_for_plugin(current_plugin_->path);
            run_lua_file(resolved, "include");
        });

    lua_["aetherius"] = api;
}

void LuaPluginManager::set_package_path_for_plugin(const std::filesystem::path& plugin_dir) {
    sol::table package = lua_["package"];
    std::string path = package["path"];
    if (!path.empty() && path.back() != ';') {
        path += ";";
    }
    const std::string base = plugin_dir.string();
    path = base + "/?.lua;";
    path += base + "/?/init.lua;";
    path += base + "/server/?.lua;";
    path += base + "/shared/?.lua";
    package["path"] = path;
}

void LuaPluginManager::register_game_command(const std::string& name,
                                             const std::string& description,
                                             sol::protected_function callback) {
    if (name.empty()) {
        LOG_WARN("Lua register_command called with empty name");
        return;
    }

    const std::string plugin_name = current_plugin_ ? current_plugin_->name : std::string();
    run_game_task([this, name, description, callback = std::move(callback), plugin_name]() mutable {
        if (command_registry_.hasGameCommand(name)) {
            command_registry_.unregisterGameCommand(name);
            LOG_WARN("Lua command replaced existing command: " + name);
        }
        command_registry_.registerGameCommand(
            std::make_unique<LuaGameCommand>(name, description, std::move(callback), *this, plugin_name));
    });

    if (current_plugin_) {
        current_plugin_->registered_game_commands.push_back(name);
    }
}

void LuaPluginManager::register_hook(const std::string& event,
                                     const std::string& name,
                                     sol::protected_function callback) {
    if (!current_plugin_) {
        LOG_WARN("Lua hook_add called outside plugin context");
        return;
    }
    if (event.empty() || name.empty()) {
        LOG_WARN("Lua hook_add called with empty event or name");
        return;
    }

    auto& hooks = hooks_[event];
    hooks.erase(std::remove_if(hooks.begin(), hooks.end(),
        [&name](const LuaHook& hook) {
            return hook.hook_name == name;
        }), hooks.end());

    hooks.push_back({current_plugin_->name, name, std::move(callback)});
    current_plugin_->registered_hooks.emplace_back(event, name);
}

void LuaPluginManager::remove_hook(const std::string& event, const std::string& name) {
    auto it = hooks_.find(event);
    if (it == hooks_.end()) {
        return;
    }

    auto& hooks = it->second;
    hooks.erase(std::remove_if(hooks.begin(), hooks.end(),
        [&name](const LuaHook& hook) {
            return hook.hook_name == name;
        }), hooks.end());
}

bool LuaPluginManager::run_lua_file(const std::filesystem::path& path, const std::string& context) {
    sol::load_result chunk = lua_.load_file(path.string());
    if (!chunk.valid()) {
        sol::error err = chunk;
        LOG_ERROR("Lua " + context + " load error in " + path.string() + ": " + std::string(err.what()));
        return false;
    }

    sol::protected_function func = chunk;
    sol::protected_function_result result = func();
    if (!result.valid()) {
        sol::error err = result;
        LOG_ERROR("Lua " + context + " runtime error in " + path.string() + ": " + std::string(err.what()));
        return false;
    }

    return true;
}

bool LuaPluginManager::resolve_plugin_path(const std::filesystem::path& plugin_dir,
                                           const std::string& relative_path,
                                           std::filesystem::path& out_path) const {
    if (relative_path.empty()) {
        return false;
    }
    std::filesystem::path candidate = plugin_dir / relative_path;
    std::error_code ec;
    std::filesystem::path base = std::filesystem::weakly_canonical(plugin_dir, ec);
    std::filesystem::path resolved = std::filesystem::weakly_canonical(candidate, ec);

    const std::string base_str = base.string();
    const std::string resolved_str = resolved.string();
    if (resolved_str.compare(0, base_str.size(), base_str) != 0) {
        return false;
    }
    if (resolved_str.size() > base_str.size() && resolved_str[base_str.size()] != '/') {
        return false;
    }

    out_path = resolved;
    return true;
}

void LuaPluginManager::run_game_task(const std::function<void()>& task) {
    if (Server::get_instance().is_game_thread()) {
        task();
        return;
    }
    if (running_ && worker_.joinable() && std::this_thread::get_id() == worker_.get_id()) {
        Server::get_instance().post_game_task(task);
        return;
    }
    auto done = std::make_shared<std::promise<void>>();
    auto future = done->get_future();
    Server::get_instance().post_game_task([task, done]() mutable {
        task();
        done->set_value();
    });
    future.get();
}

void LuaPluginManager::enqueue_task(std::function<void()> task) {
    if (!running_) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push_back(std::move(task));
    }
    queue_cv_.notify_one();
}

void LuaPluginManager::run_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this]() { return !task_queue_.empty() || !running_; });
            if (!running_ && task_queue_.empty()) {
                break;
            }
            task = std::move(task_queue_.front());
            task_queue_.pop_front();
        }

        if (task) {
            task();
        }
    }
}

void LuaPluginManager::call_with_plugin(LuaPlugin* plugin, const std::function<void()>& fn) {
    LuaPlugin* previous = current_plugin_;
    current_plugin_ = plugin;
    if (current_plugin_) {
        set_package_path_for_plugin(current_plugin_->path);
    }
    fn();
    current_plugin_ = previous;
}
