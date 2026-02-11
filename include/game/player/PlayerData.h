#pragma once

#include <string>

// Forward declare Player class to avoid circular dependencies in headers that include this one.
// The full definition will be included in the .cpp file.
class Player;

class PlayerData {
public:
    static void save(const Player& player);
    static bool load(Player& player);
};
