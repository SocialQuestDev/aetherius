#pragma once

#include <string>
#include <memory>

class Connection; // Forward declaration

class Player {
public:
    Player(int id, std::string nickname, std::shared_ptr<Connection> connection);

    // Getters
    int getId() const;
    std::string getNickname() const;
    std::shared_ptr<Connection> getConnection() const;
    double getX() const;
    double getY() const;
    double getZ() const;
    float getYaw() const;
    float getPitch() const;
    float getHealth() const;
    int getFood() const;
    bool isDead() const;
    bool isOnGround() const;

    // Setters
    void setPosition(double x, double y, double z);
    void setRotation(float yaw, float pitch);
    void setHealth(float health);
    void setFood(int food);
    void setDead(bool dead);
    void setOnGround(bool onGround);

    // Actions
    void kill();
    void teleportToSpawn();

private:
    int id;
    std::string nickname;
    std::shared_ptr<Connection> connection;

    // Player state
    double x, y, z;
    float yaw, pitch;
    float health;
    int food;
    bool dead;
    bool onGround;
};