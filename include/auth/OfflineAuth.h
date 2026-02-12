#pragma once

#include "Auth.h"

class OfflineAuth : public Auth {
public:
    OfflineAuth();
    ~OfflineAuth() override;

    PlayerProfile getPlayerProfile(const std::string& nickname) override;
    std::vector<uint8_t> generateVerifyToken() override;
    std::string calculateServerHash(
        const std::string& serverId,
        const std::vector<uint8_t>& secret,
        const std::vector<uint8_t>& pubKey
    ) override;
};
