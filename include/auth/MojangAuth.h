#pragma once

#include "Auth.h"
#include <string>
#include <vector>

class MojangAuth : public Auth {
public:
    MojangAuth();
    ~MojangAuth() override;

    PlayerProfile getPlayerProfile(const std::string& nickname) override;
    std::vector<uint8_t> generateVerifyToken() override;
    std::string calculateServerHash(
        const std::string& serverId,
        const std::vector<uint8_t>& secret,
        const std::vector<uint8_t>& pubKey
    ) override;

private:
    static std::string generateUuidUrl(const std::string& nickname);
    static std::string generateProfileUrl(const std::string& uuid);
    static std::string minecraftSha1ToHex(const unsigned char* hash);
};
