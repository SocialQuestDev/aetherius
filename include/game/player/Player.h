#pragma once

#include <string>
#include <memory>
#include <vector>

#include "../../auth/UUID.h"
#include "../../other/Vector3.h"
#include "../../other/Vector2.h"

class Connection; // Forward declaration

// Placeholder for an item stack
struct Slot {
    int itemId = 0;
    int count = 0;
};

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
    short getHeldItemSlot() const;
    const std::vector<Slot>& getInventory() const;
    bool isFlying() const;
    uint8_t getViewDistance() const;
    bool isSneaking() const;
    bool isSprinting() const;

    // Setters
    void setPosition(const Vector3& position);
    void setRotation(const Vector2& rotation);
    void setHealth(float health);
    void setFood(int food);
    void setDead(bool dead);
    void setOnGround(bool onGround);
    void setHeldItemSlot(short slot);
    void setInventorySlot(int slot, const Slot& item);
    void setFlying(bool flying);
    void setViewDistance(uint8_t viewDistance);
    void setSneaking(bool sneaking);
    void setSprinting(bool sprinting);


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
    short heldItemSlot;
    std::vector<Slot> inventory;
    bool flying;
    uint8_t viewDistance;
    bool sneaking;
    bool sprinting;
};