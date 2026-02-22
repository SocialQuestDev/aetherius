#include <algorithm>
#include <future>
#include <iostream>
#include <dlfcn.h>
#include <cassert>
#include "nethost.h"
#include "hostfxr.h"
#include "coreclr_delegates.h"
#include "plugins/SharpPluginManager.h"
#include "console/Logger.h"
#include "Server.h"
#include "game/player/PlayerList.h"

#ifdef _UNICODE
#define STR(x) L##x
#else
#define STR(x) x
#endif

#ifndef UNMANAGEDCALLERSONLY_METHOD
#define UNMANAGEDCALLERSONLY_METHOD ((const char_t*)-1)
#endif

// api to managed code
struct NativeApi {
    void (*log)(const char*, size_t, uint8_t); // log(const char* message, size_t length, uint8_t type);
};

void* load_library(const char* path)
{
    void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!h)
        throw std::runtime_error(dlerror());

    return h;
}

template<typename T>
T get_symbol(void* lib, const char* name)
{
    auto sym = dlsym(lib, name);
    if (!sym)
        throw std::runtime_error(dlerror());

    return reinterpret_cast<T>(sym);
}

void log(const char* message, size_t length, uint8_t type) {
    const std::string s(message, length);

    switch (type) {
        default:
        case 0:
            LOG_INFO(s);
            break;
        case 1:
            LOG_DEBUG(s);
            break;
        case 2:
            LOG_WARN(s);
            break;
        case 3:
            LOG_ERROR(s);
            break;
        case 4:
            LOG_FATAL(s);
            break;
    }
}

SharpPluginManager::SharpPluginManager(CommandRegistry& command_registry)
    : command_registry_(command_registry),
      plugins_dir_(std::filesystem::current_path() / "plugins_sharp") {
}

void SharpPluginManager::start()
{
    void* nethost = load_library("libnethost.so");

    auto get_hostfxr_path =
        get_symbol<int(*)(char_t*, size_t*, const struct get_hostfxr_parameters*)>(
            nethost,
            "get_hostfxr_path");

    char_t buffer[2048];
    size_t buffer_size = sizeof(buffer);

    int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
    if (rc != 0)
        throw std::runtime_error("Failed to resolve hostfxr path");

    void* hostfxr = load_library(buffer);

    auto init_f =
        get_symbol<hostfxr_initialize_for_runtime_config_fn>(
            hostfxr,
            "hostfxr_initialize_for_runtime_config");

    auto get_delegate =
        get_symbol<hostfxr_get_runtime_delegate_fn>(
            hostfxr,
            "hostfxr_get_runtime_delegate");

    auto close_f =
        get_symbol<hostfxr_close_fn>(
            hostfxr,
            "hostfxr_close");

    std::filesystem::path config_path =
        std::filesystem::absolute(
            std::filesystem::current_path() /
            "Aetherius.runtimeconfig.json");

    hostfxr_handle ctx = nullptr;

    hostfxr_initialize_parameters params {};
    params.size = sizeof(hostfxr_initialize_parameters);

    params.dotnet_root = STR("/usr/share/dotnet");

    rc = init_f(config_path.c_str(), &params, &ctx);
    if (rc != 0)
    {
        LOG_FATAL("hostfxr init failed rc=" + std::to_string(rc));
        throw std::runtime_error("hostfxr init failed");
    }

    load_assembly_and_get_function_pointer_fn load_assembly = nullptr;

    rc = get_delegate(
        ctx,
        hdt_load_assembly_and_get_function_pointer,
        (void**)&load_assembly);

    if (rc != 0)
    {
        close_f(ctx);
        throw std::runtime_error("delegate resolve failed");
    }

    using init_fn =
        void(*)(const char*, size_t, NativeApi*, ManagedApi*);

    init_fn managed_init = nullptr;

    std::filesystem::path managed_dll_path = std::filesystem::current_path() / "ManagedHost.dll";

    rc = load_assembly(
        managed_dll_path.c_str(),
        STR("ManagedHost.Entry, ManagedHost"),
        STR("Init"),
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        (void**)&managed_init);

    close_f(ctx);

    if (rc != 0 || !managed_init)
        throw std::runtime_error("managed bootstrap failed");

    std::string pluginsDir =
        (std::filesystem::current_path() / "plugins_sharp")
        .generic_string();

    static NativeApi native_api{ &log };
    ManagedApi managed_api{};

    managed_init(
        pluginsDir.c_str(),
        pluginsDir.size(),
        &native_api,
        &managed_api);

    managed_api_ =
        std::make_unique<ManagedApi>(managed_api);
}

void SharpPluginManager::stop() {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return;
    }

    managed_api_->stop();
    managed_api_ = nullptr;
}

void SharpPluginManager::load_all() {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return;
    }

    managed_api_->load_all();
}

void SharpPluginManager::unload_all() {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return;
    }

    managed_api_->unload_all();
}

void SharpPluginManager::reload_all() {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return;
    }

    managed_api_->reload_all();
}

void SharpPluginManager::tick() {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return;
    }

    managed_api_->tick();
}

void SharpPluginManager::emit_server_start() {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return;
    }

    managed_api_->emit_server_start();
}

void SharpPluginManager::emit_server_stop() {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return;
    }

    managed_api_->emit_server_stop();
}

void SharpPluginManager::emit_player_join(const PluginPlayer &player) {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return;
    }

    const auto rawPlayer = const_cast<PluginPlayer*>(&player);
    managed_api_->emit_player_join(rawPlayer);
}

void SharpPluginManager::emit_player_leave(const PluginPlayer &player, const std::string &reason) {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return;
    }

    const auto rawPlayer = const_cast<PluginPlayer*>(&player);

    managed_api_->emit_player_leave(rawPlayer, reason.c_str(), reason.length());
}

bool SharpPluginManager::emit_player_chat(const PluginPlayer &player, std::string &message) {
    if (!managed_api_) {
        LOG_ERROR("Managed API is not loaded!");
        return false;
    }

    const auto rawPlayer = const_cast<PluginPlayer*>(&player);

    return managed_api_->emit_player_chat(rawPlayer, message.c_str(), message.length());
}