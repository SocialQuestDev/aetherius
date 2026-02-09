#include "../../../include/game/player/Player.h"
#include "../../../include/network/Connection.h"
#include "../../../include/network/packet/play/UpdateHealthPacket.h"
#include "../../../include/network/packet/play/PlayerPositionAndLookPacket.h"
#include "../../../include/network/packet/play/EntityStatusPacket.h"
#include "../../../include/network/packet/play/ChatMessagePacket.h"
#include "../../../include/auth/MojangAuthHelper.h"
#include "../../../include/Logger.h"

Player::Player(int id, std::string nickname, std::shared_ptr<Connection> connection)
    : id(id), nickname(std::move(nickname)), connection(connection),
      x(0.0), y(7.0), z(0.0), yaw(0.0f), pitch(0.0f), health(20.0f), food(20), dead(false), onGround(false) {}

int Player::getId() const {
    return id;
}

std::string Player::getNickname() const {
    return nickname;
}

std::shared_ptr<Connection> Player::getConnection() const {
    return connection;
}

double Player::getX() const {
    return x;
}

double Player::getY() const {
    return y;
}

double Player::getZ() const {
    return z;
}

float Player::getYaw() const {
    return yaw;
}

float Player::getPitch() const {
    return pitch;
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

void Player::setPosition(double x, double y, double z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

void Player::setRotation(float yaw, float pitch) {
    this->yaw = yaw;
    this->pitch = pitch;
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
    x = 0.0;
    y = 7.0;
    z = 0.0;
    yaw = 0.0f;
    pitch = 0.0f;
    onGround = false;

    UpdateHealthPacket healthPacket(health, food, 5.0f);
    connection->send_packet(healthPacket);

    PlayerPositionAndLookPacket posLookPacket(x, y, z, yaw, pitch, 0x00, 1);
    connection->send_packet(posLookPacket);

    LOG_DEBUG("Player " + nickname + " teleported to spawn and healed");
}
