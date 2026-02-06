#include "../../../include/game/player/PlayerList.h"

Player* PlayerList::get_player(const int id) const {
    if (id < 0 || id >= maxPlayers)
        return nullptr;

    return players[id];
}

void PlayerList::add_player(int id, std::string& nickname, std::string& uuid, std::string& textures) {
    players[id] = new Player(id, nickname, uuid, textures);
    playersCount++;
}

void PlayerList::add_player(std::string& nickname, std::string& uuid, std::string& textures) {
    add_player(playersCount, nickname, uuid, textures);
}

void PlayerList::remove_player(const int id) {
    if (id < 0 || id >= maxPlayers)
        return;

    if (players[id] != nullptr) {
        delete players[id];
        players[id] = nullptr;
        playersCount--;
    }
}

int PlayerList::get_players_count() const {
    return playersCount;
}

void PlayerList::set_players_count(const int count) {
    playersCount = count;
}

int PlayerList::get_max_players() const {
    return maxPlayers;
}

void PlayerList::set_max_players(int count) {
    auto playersTemp = std::make_unique<Player*[]>(count);

    int copyCount = std::min(maxPlayers, count);

    for (int i = 0; i < copyCount; i++) {
        playersTemp[i] = players[i];
    }

    for (int i = copyCount; i < count; i++) {
        playersTemp[i] = nullptr;
    }

    maxPlayers = count;
    players = std::move(playersTemp);
}
