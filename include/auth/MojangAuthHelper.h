#pragma once
#include "UUID.h"

#include <vector>
#include <boost/multiprecision/cpp_int.hpp>

struct PlayerData {
    UUID uuid;
    std::string textures;
};

namespace auth {
    std::vector<uint8_t> generate_verify_token();
    std::string generate_uuid_url(const std::string& nickname);
    std::string generate_profile_url(const std::string& uuid);
    PlayerData get_uuid(const std::string& nickname);
}