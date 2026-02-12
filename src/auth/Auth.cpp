#include "auth/Auth.h"
#include "auth/MojangAuth.h"
#include "auth/OfflineAuth.h"
#include <magic_enum.hpp>
#include "console/Logger.h"

std::unique_ptr<Auth> Auth::Create(const AuthType authType) {
    switch (authType) {
        case AuthType::Offline:
            return std::make_unique<OfflineAuth>();
        case AuthType::Mojang:
            return std::make_unique<MojangAuth>();
        case AuthType::ElyBy:
            LOG_ERROR("ElyBy is not implemented yet!");
            return nullptr;

        case AuthType::Our:
            LOG_ERROR("Our is not implemented yet!");
            return nullptr;

        case AuthType::Custom:
            LOG_ERROR("Custom is not implemented yet!");
            return nullptr;

        default:
            LOG_ERROR("Unsupported AuthType " + std::string(magic_enum::enum_name(authType)) + " !");
            return nullptr;
    }
}
