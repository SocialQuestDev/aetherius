#include "../../../include/game/player/PlayerList.h"

PlayerList& PlayerList::getInstance() {
    static PlayerList instance;
    return instance;
}

void PlayerList::addPlayer(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(playersMutex);
    players.push_back(player);
}

void PlayerList::removePlayer(int playerId) {
    std::lock_guard<std::mutex> lock(playersMutex);
    players.erase(
        std::remove_if(players.begin(), players.end(),
                       [playerId](const std::shared_ptr<Player>& player) {
                           return player->getId() == playerId;
                       }),
        players.end());
}

std::shared_ptr<Player> PlayerList::getPlayer(int playerId) {
    std::lock_guard<std::mutex> lock(playersMutex);
    for (const auto& player : players) {
        if (player->getId() == playerId) {
            return player;
        }
    }
    return nullptr;
}

std::shared_ptr<Player> PlayerList::getPlayer(const std::string& nickname) {
    std::lock_guard<std::mutex> lock(playersMutex);
    for (const auto& player : players) {
        if (player->getNickname() == nickname) {
            return player;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Player>> PlayerList::getPlayers() {
    std::lock_guard<std::mutex> lock(playersMutex);
    return players;
}

int PlayerList::getNextPlayerId() {
    return nextPlayerId++;
}
