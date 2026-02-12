#pragma once

#include <string>
#include <memory>
#include <vector>

#include "auth/UUID.h"
#include "other/Vector3.h"
#include "other/Vector2.h"
#include "other/ChatColor.h"
#include "auth/Auth.h"

class Connection;
class Packet;
enum class EquipmentSlot;

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
    std::unique_ptr<Auth>& getAuth();
    std::vector<std::pair<EquipmentSlot, Slot>> getFullEquipment() const;

    const std::vector<Slot>& getInventory() const;
    std::vector<Slot>& getInventory();

    bool isFlying() const;
    uint8_t getViewDistance() const;
    bool isSneaking() const;
    bool isSprinting() const;
    std::string getLocale() const;
    ChatMode getChatMode() const;
    bool hasChatColors() const;
    uint8_t getDisplayedSkinParts() const;
    MainHand getMainHand() const;

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
    void setAuth(std::unique_ptr<Auth> auth);
    void setUuid(UUID uuid);
    void setSkin(const std::string& skin);

    void kill();
    void teleportToSpawn();
    void disconnect(std::string reason = "Disconnected") const;
    void sendChatMessage(const std::string &message, ChatColor color = ChatColor::WHITE, UUID senderUuid = UUID()) const;
    void broadcastEquipmentUpdate() const;

private:
    int id;
    UUID uuid;
    std::string nickname;
    std::string skin;
    std::shared_ptr<Connection> connection;
    std::unique_ptr<Auth> auth;

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
