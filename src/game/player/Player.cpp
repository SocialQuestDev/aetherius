#include "../../../include/game/player/Player.h"
#include "../../../include/network/Connection.h"
#include "../../../include/network/packet/play/UpdateHealthPacket.h"
#include "../../../include/network/packet/play/PlayerPositionAndLookPacket.h"
#include "../../../include/network/packet/play/EntityStatusPacket.h"
#include "../../../include/network/packet/play/ChatMessagePacket.h"
#include "../../../include/auth/MojangAuthHelper.h"
#include "../../../include/Logger.h"

Player::Player(int id, UUID uuid, std::string nickname, std::string skin, std::shared_ptr<Connection> connection)
    : id(id), uuid(uuid), nickname(std::move(nickname)), skin(std::move(skin)), connection(connection),
      position(0.0, 7.0, 0.0), rotation(0.0f, 0.0f), health(20.0f), food(20), dead(false), onGround(false),
      heldItemSlot(0), inventory(46), flying(false), viewDistance(8), sneaking(false), sprinting(false) {}

int Player::getId() const {
    return id;
}

UUID Player::getUuid() const {
    return uuid;
}

std::string Player::getNickname() const {
    return nickname;
}

std::string Player::getSkin() const {
    return skin;
}

std::shared_ptr<Connection> Player::getConnection() const {
    return connection;
}

Vector3 Player::getPosition() const {
    return position;
}

Vector2 Player::getRotation() const {
    return rotation;
}

float Player::getHealth() const {
    return health;
}

int Player::getFood() const {
    return food;
}

bool Player::isDead() const {
    return dead;
}

bool Player::isOnGround() const {
    return onGround;
}

short Player::getHeldItemSlot() const {
    return heldItemSlot;
}

const std::vector<Slot>& Player::getInventory() const {
    return inventory;
}

bool Player::isFlying() const {
    return flying;
}

uint8_t Player::getViewDistance() const {
    return viewDistance;
}

bool Player::isSneaking() const {
    return sneaking;
}

bool Player::isSprinting() const {
    return sprinting;
}

void Player::setPosition(const Vector3& position) {
    this->position = position;
}

void Player::setRotation(const Vector2& rotation) {
    this->rotation = rotation;
}

void Player::setHealth(float health) {
    this->health = health;
}

void Player::setFood(int food) {
    this->food = food;
}

void Player::setDead(bool dead) {
    this->dead = dead;
}

void Player::setOnGround(bool onGround) {
    this->onGround = onGround;
}

void Player::setHeldItemSlot(short slot) {
    this->heldItemSlot = slot;
}

void Player::setInventorySlot(int slot, const Slot& item) {
    if (slot >= 0 && slot < inventory.size()) {
        inventory[slot] = item;
    }
}

void Player::setFlying(bool flying) {
    this->flying = flying;
}

void Player::setViewDistance(uint8_t viewDistance) {
    this->viewDistance = viewDistance;
}

void Player::setSneaking(bool sneaking) {
    this->sneaking = sneaking;
}

void Player::setSprinting(bool sprinting) {
    this->sprinting = sprinting;
}

void Player::kill() {
    if (dead) return;
    dead = true;

    LOG_DEBUG("Player " + nickname + " fell into the void. Sending death status and zero health.");

    UpdateHealthPacket healthPacket(0.0f, 0, 0.0f);
    connection->send_packet(healthPacket);

    EntityStatusPacket deathPacket(id, 3);
    connection->send_packet(deathPacket);

    ChatMessagePacket chatPacket("{\"text\":\"Вы упали в бездну!\", \"color\":\"red\"}", 1, get_offline_UUID_128(nickname));
    connection->send_packet(chatPacket);
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

    PlayerPositionAndLookPacket posLookPacket(position.x, position.y, position.z, rotation.x, rotation.y, 0x00, 1);
    connection->send_packet(posLookPacket);

    LOG_DEBUG("Player " + nickname + " teleported to spawn and healed");
}
