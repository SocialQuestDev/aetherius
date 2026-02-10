#pragma once

#include <string>
#include <memory>

#include "../../auth/UUID.h"
#include "../../other/Vector3.h"
#include "../../other/Vector2.h"

class Connection; // Forward declaration

class Player {
public:
    Player(int id, UUID uuid, std::string nickname, std::string skin, std::shared_ptr<Connection> connection);

    // Getters
    int getId() const;
    UUID getUuid() const;
    std::string getNickname() const;
    std::string getSkin() const;
    std::shared_ptr<Connection> getConnection() const;
    Vector3 getPosition() const;
    Vector2 getRotation() const;
    float getHealth() const;
    int getFood() const;
    bool isDead() const;
    bool isOnGround() const;

    // Setters
    void setPosition(const Vector3& position);
    void setRotation(const Vector2& rotation);
    void setHealth(float health);
    void setFood(int food);
    void setDead(bool dead);
    void setOnGround(bool onGround);

    // Actions
    void kill();
    void teleportToSpawn();

private:
    int id;
    UUID uuid;
    std::string nickname;
    std::string skin;
    std::shared_ptr<Connection> connection;

    // Player state
    Vector3 position;
    Vector2 rotation;
    float health;
    int food;
    bool dead;
    bool onGround;
};