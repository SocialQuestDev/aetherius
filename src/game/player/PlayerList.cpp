#include "../../../include/game/player/PlayerList.h"
#include "../../../include/network/packet/play/PlayerInfoPacket.h"
#include "../../../include/network/Connection.h"

PlayerList& PlayerList::getInstance() {
    static PlayerList instance;
    return instance;
}

void PlayerList::addPlayer(std::shared_ptr<Player> newPlayer) {
    std::lock_guard<std::mutex> lock(playersMutex);

    // 1. Notify the new player about all existing players
    if (!players.empty()) {
        PlayerInfoPacket existingPlayersInfo(PlayerInfoPacket::ADD_PLAYER, players);
        newPlayer->getConnection()->send_packet(existingPlayersInfo);
    }

    // 2. Notify all existing players about the new player
    PlayerInfoPacket newPlayerInfo(PlayerInfoPacket::ADD_PLAYER, {newPlayer});
    for (const auto& existingPlayer : players) {
        existingPlayer->getConnection()->send_packet(newPlayerInfo);
    }

    // 3. Add the new player to the list
    players.push_back(newPlayer);
}

void PlayerList::removePlayer(int playerId) {
    std::lock_guard<std::mutex> lock(playersMutex);
    // TODO: Send PlayerInfo REMOVE_PLAYER packet to all clients
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
    nextPlayerId =+ 1;
    return nextPlayerId;
}
