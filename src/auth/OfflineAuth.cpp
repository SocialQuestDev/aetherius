#include "auth/OfflineAuth.h"
#include "auth/UUID.h"
#include <stdexcept>

OfflineAuth::OfflineAuth() = default;
OfflineAuth::~OfflineAuth() = default;

Auth::PlayerProfile OfflineAuth::getPlayerProfile(const std::string& nickname) {
    return {get_offline_UUID_128(nickname), ""};
}

std::vector<uint8_t> OfflineAuth::generateVerifyToken() {
    return {};
}

std::string OfflineAuth::calculateServerHash(
    const std::string& serverId,
    const std::vector<uint8_t>& secret,
    const std::vector<uint8_t>& pubKey
) {
    return "";
}
