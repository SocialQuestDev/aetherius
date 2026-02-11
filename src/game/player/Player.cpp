#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/Connection.h"
#include "network/packet/outbound/play/UpdateHealthPacket.h"
#include "network/packet/outbound/play/PlayerPositionAndLookPacket.h"
#include "network/packet/outbound/play/EntityStatusPacket.h"
#include "network/packet/outbound/play/ChatMessagePacket.h"
#include "network/packet/outbound/play/PlayerInfoPacket.h"
#include "network/packet/outbound/play/DestroyEntitiesPacket.h"
#include "auth/MojangAuthHelper.h"
#include "console/Logger.h"
#include <memory>

#include "network/Metadata.h"
#include "network/packet/outbound/play/EntityEquipmentPacket.h"
#include "network/packet/outbound/play/EntityMetadataPacket.h"

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
Gamemode Player::getGamemode() const {return gamemode; }
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
void Player::setGamemode(Gamemode gamemode) { this->gamemode = gamemode; }

void Player::kill() {
    if (dead) return;
    dead = true;

    // 1. Отправляем самому игроку, что он мертв (экран смерти)
    UpdateHealthPacket healthPacket(0.0f, 0, 0.0f);
    connection->send_packet(healthPacket);

    // Подготавливаем метадату
    Metadata meta;
    meta.addFloat(9, 0.0f); // Здоровье в 0 (Index 9 из скриншота Living Entity)
    meta.addPose(6, 2);  // Опционально: поза смерти, если 0.0f не хватит

    // Создаем пакет (код пакета SetEntityMetadata обычно 0x56 или 0x50)
    EntityMetadataPacket metadataPacket(id, meta);

    // 2. Рассылаем всем статус
    EntityStatusPacket damagePacket(id, static_cast<std::byte>(2)); // Покраснение
    EntityStatusPacket deathPacket(id, static_cast<std::byte>(3));  // Падение на бок

    for (const auto& other : PlayerList::getInstance().getPlayers()) {
        // Не отправляем самому себе (опционально, зависит от версии)
        other->getConnection()->send_packet(metadataPacket);
        other->getConnection()->send_packet(damagePacket);
        other->getConnection()->send_packet(deathPacket);
    }

    // 3. Сообщение в чат
    for (const auto& other : PlayerList::getInstance().getPlayers()) {
        other->sendChatMessage(nickname + " умер!");
    }

    // ВАЖНО: Через ~1-2 секунды нужно отправить пакет Destroy Entities (код 0x3D или аналогичный)
    // Иначе «труп» так и будет лежать или стоять в зависимости от версии клиента.

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

    // Clear inventory and reinitialize to correct size (46 slots)
    inventory.clear();
    inventory.resize(46);

    EntityEquipmentPacket equipPacket(id, 0, Slot());
    for (const auto& p : PlayerList::getInstance().getPlayers()) {
        p->connection->send_packet(equipPacket);
    }

    LOG_DEBUG("Player " + nickname + " teleported to spawn and healed");
}

void Player::disconnect() const {
    // Проверяем, есть ли игрок в списке (избегаем повторного отключения)
    if (!PlayerList::getInstance().getPlayer(getId())) {
        return;
    }

    // Сначала удаляем игрока из списка, чтобы избежать повторного вызова disconnect
    PlayerList::getInstance().removePlayer(getId());

    // Собираем список оставшихся игроков
    const auto& remainingPlayers = PlayerList::getInstance().getPlayers();

    // Отправляем сообщения и пакеты оставшимся игрокам
    for (const auto& client : remainingPlayers) {
        client->sendChatMessage(nickname + " left the game", ChatColor::YELLOW);
    }

    DestroyEntitiesPacket destroyPacket({getId()});
    for (const auto& p : remainingPlayers) {
        p->getConnection()->send_packet(destroyPacket);
    }

    // Закрываем соединение
    boost::system::error_code ec;
    connection->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    connection->socket().close(ec);

    LOG_DEBUG("Player " + nickname + " disconnected");
}

void Player::sendChatMessage(const std::string &message, const ChatColor color, const UUID senderUuid) const {
    ChatMessagePacket message_packet = ChatMessagePacket("{\"text\":\"" + message + "\", \"color\":\"" + to_string(color) + "\"}", 1, senderUuid);
    connection->send_packet(message_packet);
}
