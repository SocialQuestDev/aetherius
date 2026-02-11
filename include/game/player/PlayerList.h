#pragma once

#include "game/player/Player.h"
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <mutex>
#include <atomic>

class PlayerList {
public:
    static PlayerList& getInstance();

    void addPlayer(std::shared_ptr<Player> player);
    void removePlayer(int playerId);
    std::shared_ptr<Player> getPlayer(int playerId);
    std::shared_ptr<Player> getPlayer(const std::string& nickname);

    std::vector<std::shared_ptr<Player>> getPlayers();
    int getNextPlayerId();

private:
    PlayerList() : nextPlayerId(1) {}
    PlayerList(const PlayerList&) = delete;
    PlayerList& operator=(const PlayerList&) = delete;

    std::vector<std::shared_ptr<Player>> players;
    std::mutex playersMutex;
    std::atomic<int> nextPlayerId;
};
