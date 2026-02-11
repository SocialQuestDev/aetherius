#pragma once

#include <string>
#include <memory>
#include <vector>

#include "auth/UUID.h"
#include "other/Vector3.h"
#include "other/Vector2.h"
#include "other/ChatColor.h"

// Forward declare to avoid circular dependencies
class Connection;
class Packet;

struct Slot {
    int itemId = 0;
    int count = 0;
};

enum class ChatMode {
    ENABLED,
    COMMANDS_ONLY,
    HIDDEN
};

enum class MainHand {
    LEFT,
    RIGHT
};

enum class Gamemode {
    SURVIVAL = 0,
    CREATIVE = 1,
    ADVENTURE = 2,
    SPECTATOR = 3
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
    Gamemode getGamemode() const;

    // Overloaded getInventory()
    const std::vector<Slot>& getInventory() const; // For read-only access
    std::vector<Slot>& getInventory();             // For read-write access

    bool isFlying() const;
    uint8_t getViewDistance() const;
    bool isSneaking() const;
    bool isSprinting() const;
    std::string getLocale() const;
    ChatMode getChatMode() const;
    bool hasChatColors() const;
    uint8_t getDisplayedSkinParts() const;
    MainHand getMainHand() const;

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
    void setLocale(const std::string& locale);
    void setChatMode(ChatMode chatMode);
    void setChatColors(bool chatColors);
    void setDisplayedSkinParts(uint8_t displayedSkinParts);
    void setMainHand(MainHand mainHand);
    void setGamemode(Gamemode gamemode);

    // Actions
    void kill();
    void teleportToSpawn();
    void disconnect() const;
    void sendChatMessage(const std::string &message, ChatColor color = ChatColor::WHITE, UUID senderUuid = UUID()) const;

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
    std::string locale;
    ChatMode chatMode;
    bool chatColors;
    uint8_t displayedSkinParts;
    MainHand mainHand;
    Gamemode gamemode;
};
