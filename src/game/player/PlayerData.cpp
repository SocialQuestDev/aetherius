#include "../../../include/game/player/PlayerData.h"
#include "../../../include/auth/MojangAuthHelper.h"
#include "../../../include/auth/UUID.h"
#include <fstream>
#include <iostream>
#include <filesystem>

#include "game/player/Player.h"
#include "other/Vector3.h"

namespace fs = std::filesystem;

void PlayerData::save(const Player& player) {
    fs::create_directory("playerdata");
    UUID uuid = player.getUuid();
    std::ofstream file("playerdata/" + uuid_to_string(uuid.high, uuid.low) + ".dat");
    if (file.is_open()) {
        Vector3 pos = player.getPosition();
        file << pos.x << std::endl;
        file << pos.y << std::endl;
        file << pos.z << std::endl;
    }
}

bool PlayerData::load(Player& player) {
    UUID uuid = player.getUuid();
    std::ifstream file("playerdata/" + uuid_to_string(uuid.high, uuid.low) + ".dat");
    if (file.is_open()) {
        double x, y, z;
        if (file >> x >> y >> z) {
            player.setPosition({x, y, z});
            return true;
        }
    }
    return false;
}
