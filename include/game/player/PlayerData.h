#pragma once

#include <string>

class Player;

class PlayerData {
public:
    static void save(const Player& player);
    static bool load(Player& player);
};
