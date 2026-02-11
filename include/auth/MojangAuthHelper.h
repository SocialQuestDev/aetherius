#pragma once
#include "auth/UUID.h"

#include <vector>
#include <boost/multiprecision/cpp_int.hpp>

struct PlayerMg {
    UUID uuid;
    std::string textures;
};

namespace auth {
    std::vector<uint8_t> generate_verify_token();
    std::string generate_uuid_url(const std::string& nickname);
    std::string generate_profile_url(const std::string& uuid);
    PlayerMg get_uuid(const std::string& nickname);
}