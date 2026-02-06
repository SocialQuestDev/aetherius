#pragma once
#include <memory>
#include "../../auth/UUID.h"

class Player {
private:
    int id;
    std::string uuid;
    std::string nickname;
    std::string textures;
public:
    int get_id() const;
    std::string get_uuid();
    std::string get_nickname();
    std::string get_textures();

    Player(int id, std::string nick, std::string uuid, std::string textures)
    : id(id), nickname(std::move(nick)), uuid(uuid), textures(textures)
    {

    }
};