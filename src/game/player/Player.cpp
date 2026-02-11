#include "../../../include/game/player/Player.h"
#include "../../../include/game/player/PlayerList.h"
#include "../../../include/network/Connection.h"
#include "../../../include/network/packet/play/UpdateHealthPacket.h"
#include "../../../include/network/packet/play/PlayerPositionAndLookPacket.h"
#include "../../../include/network/packet/play/EntityStatusPacket.h"
#include "../../../include/network/packet/play/ChatMessagePacket.h"
#include "../../../include/network/packet/play/PlayerInfoPacket.h"
#include "../../../include/network/packet/play/DestroyEntitiesPacket.h"
#include "../../../include/auth/MojangAuthHelper.h"
#include "../../../include/console/Logger.h"
#include <memory>

Player::Player(int id, UUID uuid, std::string nickname, std::string skin, std::shared_ptr<Connection> connection)
    : id(id), uuid(uuid), nickname(std::move(nickname)), skin(std::move(skin)), connection(connection),
      position(0.0, 7.0, 0.0), rotation(0.0f, 0.0f), health(20.0f), food(20), dead(false), onGround(false),
      heldItemSlot(0), inventory(46), flying(false), viewDistance(8), sneaking(false), sprinting(false),
      locale("en_US"), chatMode(ChatMode::ENABLED), chatColors(true), displayedSkinParts(0x7F), mainHand(MainHand::RIGHT) {}

// Getters
int Player::getId() const { return id; }
UUID Player::getUuid() const { return uuid; }
std::string Player::getNickname() const { return nickname; }
std::string Player::getSkin() const { return skin; }
std::shared_ptr<Connection> Player::getConnection() const { return connection; }
Vector3 Player::getPosition() const { return position; }
Vector2 Player::getRotation() const { return rotation; }
float Player::getHealth() const { return health; }
int Player::getFood() const { return food; }
bool Player::isDead() const { return dead; }
bool Player::isOnGround() const { return onGround; }
short Player::getHeldItemSlot() const { return heldItemSlot; }
const std::vector<Slot>& Player::getInventory() const { return inventory; }
bool Player::isFlying() const { return flying; }
uint8_t Player::getViewDistance() const { return viewDistance; }
bool Player::isSneaking() const { return sneaking; }
bool Player::isSprinting() const { return sprinting; }
std::string Player::getLocale() const { return locale; }
ChatMode Player::getChatMode() const { return chatMode; }
bool Player::hasChatColors() const { return chatColors; }
uint8_t Player::getDisplayedSkinParts() const { return displayedSkinParts; }
MainHand Player::getMainHand() const { return mainHand; }

// Setters
void Player::setPosition(const Vector3& position) { this->position = position; }
void Player::setRotation(const Vector2& rotation) { this->rotation = rotation; }
void Player::setHealth(float health) { this->health = health; }
void Player::setFood(int food) { this->food = food; }
void Player::setDead(bool dead) { this->dead = dead; }
void Player::setOnGround(bool onGround) { this->onGround = onGround; }
void Player::setHeldItemSlot(short slot) { this->heldItemSlot = slot; }
void Player::setInventorySlot(int slot, const Slot& item) { if (slot >= 0 && slot < inventory.size()) { inventory[slot] = item; } }
void Player::setFlying(bool flying) { this->flying = flying; }
void Player::setViewDistance(uint8_t viewDistance) { this->viewDistance = viewDistance; }
void Player::setSneaking(bool sneaking) { this->sneaking = sneaking; }
void Player::setSprinting(bool sprinting) { this->sprinting = sprinting; }
void Player::setLocale(const std::string& locale) { this->locale = locale; }
void Player::setChatMode(ChatMode chatMode) { this->chatMode = chatMode; }
void Player::setChatColors(bool chatColors) { this->chatColors = chatColors; }
void Player::setDisplayedSkinParts(uint8_t displayedSkinParts) { this->displayedSkinParts = displayedSkinParts; }
void Player::setMainHand(MainHand mainHand) { this->mainHand = mainHand; }

void Player::kill() {
    if (dead) return;
    dead = true;

    LOG_DEBUG("Player " + nickname + " died. Broadcasting death animation.");

    UpdateHealthPacket healthPacket(0.0f, 0, 0.0f);
    connection->send_packet(healthPacket);

    EntityStatusPacket damagePacket(id, static_cast<std::byte>(2));
    EntityStatusPacket deathPacket(id, static_cast<std::byte>(3));
    EntityStatusPacket deathAnimPacket(id, static_cast<std::byte>(60));
    for (const auto& other : PlayerList::getInstance().getPlayers()) {
        other->getConnection()->send_packet(damagePacket);
        other->getConnection()->send_packet(deathPacket);
        other->getConnection()->send_packet(deathAnimPacket);
    }

    for (const auto& other : PlayerList::getInstance().getPlayers()) {
        other->sendChatMessage(nickname + " умер!");
    }
}

void Player::teleportToSpawn() {
    dead = false;
    health = 20.0f;
    food = 20;
    position = Vector3(0.0, 75.0, 0.0);
    rotation = Vector2(0.0f, 0.0f);
    onGround = false;
    flying = false;
    sneaking = false;
    sprinting = false;

    UpdateHealthPacket healthPacket(health, food, 5.0f);
    connection->send_packet(healthPacket);

    PlayerPositionAndLookPacket posLookPacket(position.x, position.y, position.z, rotation.x, rotation.y, 0x00, id);
    connection->send_packet(posLookPacket);

    LOG_DEBUG("Player " + nickname + " teleported to spawn and healed");
}

void Player::disconnect() const {
    for (const auto& client: PlayerList::getInstance().getPlayers()) {
        client->sendChatMessage(nickname + " left the game", ChatColor::YELLOW);
    }

    DestroyEntitiesPacket destroyPacket({getId()});
    for (const auto& p : PlayerList::getInstance().getPlayers()) {
        if (p->getId() != getId()) {
            p->getConnection()->send_packet(destroyPacket);
        }
    }

    PlayerList::getInstance().removePlayer(getId());
    connection->socket().close();

    LOG_DEBUG("Player " + nickname + " disconnected");
}

void Player::sendChatMessage(const std::string &message, const ChatColor color, const UUID senderUuid) const {
    ChatMessagePacket message_packet = ChatMessagePacket("{\"text\":\"" + message + "\", \"color\":\"" + to_string(color) + "\"}", 1, senderUuid);
    connection->send_packet(message_packet);
}
