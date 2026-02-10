#include "../../../include/game/player/PlayerList.h"
#include "../../../include/network/packet/play/PlayerInfoPacket.h"
#include "../../../include/network/Connection.h"

PlayerList& PlayerList::getInstance() {
    static PlayerList instance;
    return instance;
}

void PlayerList::addPlayer(std::shared_ptr<Player> newPlayer) {
    players.push_back(newPlayer);
}

void PlayerList::removePlayer(int playerId) {
    std::lock_guard<std::mutex> lock(playersMutex);

    auto playerIt = std::find_if(players.begin(), players.end(),
                                 [playerId](const std::shared_ptr<Player>& player) {
                                     return player->getId() == playerId;
                                 });

    if (playerIt != players.end()) {
        PlayerInfoPacket removePacket(PlayerInfoPacket::REMOVE_PLAYER, {*playerIt});
        for (const auto& p : players) {
            if (p->getId() != playerId) {
                p->getConnection()->send_packet(removePacket);
            }
        }
        players.erase(playerIt);
    }
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
