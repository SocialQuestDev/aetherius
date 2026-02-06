#include "../../../include/game/player/Player.h"

int Player::get_id() const {
    return id;
}

std::string Player::get_uuid() {
    return uuid;
}

std::string Player::get_nickname() {
    return nickname;
}

std::string Player::get_textures() {
    return textures;
}