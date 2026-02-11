#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/PlayerInfoPacket.h"
#include "network/packet/outbound/play/SpawnNamedEntityPacket.h"
#include "network/packet/outbound/play/EntityMetadataPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"

PlayerList& PlayerList::getInstance() {
    static PlayerList instance;
    return instance;
}

void PlayerList::addPlayer(std::shared_ptr<Player> newPlayer) {
    players.push_back(newPlayer);
}

void PlayerList::removePlayer(int playerId) {
    std::shared_ptr<Player> playerToRemove;
    
    {
        std::lock_guard<std::mutex> lock(playersMutex);
        
        auto it = std::find_if(players.begin(), players.end(),
            [playerId](const auto& p) { return p->getId() == playerId; });

        if (it == players.end()) return;

        playerToRemove = *it;

        PlayerInfoPacket removePacket(PlayerInfoPacket::REMOVE_PLAYER, {playerToRemove});
        for (const auto& p : players) {
            if (p->getId() != playerId) {
                p->getConnection()->send_packet(removePacket);
            }
        }
        players.erase(it);
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
    return nextPlayerId++;
}
