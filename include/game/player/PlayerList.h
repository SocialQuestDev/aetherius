#pragma once
#include "Player.h"

#include <memory>
#include <toml++/toml.hpp>

class PlayerList {
public:
    Player* get_player(int id) const;
    void add_player(int id, std::string& nickname, std::string& uuid, std::string& textures);
    void add_player(std::string& nickname, std::string& uuid, std::string& textures);
    void remove_player(int id);

    int get_players_count() const;
    void set_players_count(int count);

    int get_max_players() const;
    void set_max_players(int count);

    static PlayerList& get_instance() {
        static PlayerList instance;
        return instance;
    }
private:
    int playersCount;
    int maxPlayers;
    std::unique_ptr<Player*[]> players;

    PlayerList() {
        auto config = toml::parse_file("config.toml");

        playersCount = 0;
        maxPlayers = config["server"]["max_players"].value_or(20);

        players = std::make_unique<Player*[]>(maxPlayers);

        for (int i = 0; i < maxPlayers; ++i) {
            players[i] = nullptr;
        }
    }

    PlayerList(const PlayerList&) = delete;
    PlayerList& operator=(const PlayerList&) = delete;
};
