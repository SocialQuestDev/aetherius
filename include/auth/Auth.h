#pragma once

#include <memory>
#include <string>
#include <vector>
#include "auth/UUID.h"

enum class AuthType {
    Offline,
    Mojang,
    ElyBy,
    Our,
    Custom // for future
};

class Auth {
public:
    struct PlayerProfile {
        UUID uuid;
        std::string textures;
    };

    virtual ~Auth() = default;

    virtual PlayerProfile getPlayerProfile(const std::string& nickname) = 0;
    virtual std::vector<uint8_t> generateVerifyToken() = 0;
    virtual std::string calculateServerHash(
        const std::string& serverId,
        const std::vector<uint8_t>& secret,
        const std::vector<uint8_t>& pubKey
    ) = 0;

    static std::unique_ptr<Auth> Create(AuthType authType);
};
