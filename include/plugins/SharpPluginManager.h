#pragma once
#include <filesystem>
#include <string>
#include "commands/CommandRegistry.h"
#include "plugins/IPluginManager.h"

// api from managed code
struct ManagedApi {
    void (*stop)();

    void (*load_all)(); // load_all();
    void (*unload_all)(); // unload_all();
    void (*reload_all)(); // reload_all();
    void (*tick)(); // tick();

    void (*emit_server_start)(); // emit_server_start();
    void (*emit_server_stop)(); // emit_server_stop();
    void (*emit_player_join)(PluginPlayer*); // emit_player_join(PluginPlayer* player);
    void (*emit_player_leave)(PluginPlayer*, const char*, size_t); // emit_player_leave(PluginPlayer* player, const char* reason, size_t length);
    bool (*emit_player_chat)(PluginPlayer*, const char*, size_t); // emit_player_chat(PluginPlayer* player, const char* message, size_t length);
};

class SharpPluginManager : public IPluginManager {
public:
    explicit SharpPluginManager(CommandRegistry& command_registry);

    void start() override;
    void stop() override;

    void load_all() override;
    void unload_all() override;
    void reload_all() override;
    void tick() override;
    void emit_server_start() override;
    void emit_server_stop() override;
    void emit_player_join(const PluginPlayer& player) override;
    void emit_player_leave(const PluginPlayer& player, const std::string& reason) override;
    bool emit_player_chat(const PluginPlayer& player, std::string& message) override;
private:
    CommandRegistry& command_registry_;
    std::filesystem::path plugins_dir_;
    std::unique_ptr<ManagedApi> managed_api_;
};