#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/Connection.h"
#include "network/packet/outbound/play/UpdateHealthPacket.h"
#include "network/packet/outbound/play/PlayerPositionAndLookPacket.h"
#include "network/packet/outbound/play/EntityStatusPacket.h"
#include "network/packet/outbound/play/ChatMessagePacket.h"
#include "network/packet/outbound/play/PlayerInfoPacket.h"
#include "network/packet/outbound/play/DestroyEntitiesPacket.h"
#include "console/Logger.h"
#include <memory>

#include "network/Metadata.h"
#include "network/packet/outbound/play/EntityEquipmentPacket.h"
#include "network/packet/outbound/play/EntityMetadataPacket.h"
#include "game/player/PlayerData.h"
#include "network/packet/outbound/play/DisconnectPacketPlay.h"
#include <algorithm>

Player::Player(int id, UUID uuid, std::string nickname, std::string skin, std::shared_ptr<Connection> connection)
    : id(id), uuid(uuid), nickname(std::move(nickname)), skin(std::move(skin)), connection(connection),
      inventory(46) {
    if (!PlayerData::load(*this)) {
        // New player, set defaults and save initial data
        position = {0.0, 75.0, 0.0};
        rotation = {0.0f, 0.0f};
        health = 20.0f;
        food = 20;
        dead = false;
        onGround = false;
        heldItemSlot = 0;
        flying = false;
        viewDistance = 8;
        sneaking = false;
        sprinting = false;
        locale = "en_US";
        chatMode = ChatMode::ENABLED;
        chatColors = true;
        displayedSkinParts = 0x7F;
        mainHand = MainHand::RIGHT;
        gamemode = Gamemode::SURVIVAL;
        PlayerData::save(*this);
    }
}

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
std::vector<Slot>& Player::getInventory() { return inventory; }
bool Player::isFlying() const { return flying; }
uint8_t Player::getViewDistance() const { return viewDistance; }
bool Player::isSneaking() const { return sneaking; }
bool Player::isSprinting() const { return sprinting; }
std::string Player::getLocale() const { return locale; }
ChatMode Player::getChatMode() const { return chatMode; }
bool Player::hasChatColors() const { return chatColors; }
uint8_t Player::getDisplayedSkinParts() const { return displayedSkinParts; }
MainHand Player::getMainHand() const { return mainHand; }
std::unique_ptr<Auth>& Player::getAuth() { return auth; }

std::vector<std::pair<EquipmentSlot, Slot>> Player::getFullEquipment() const {
    std::vector<std::pair<EquipmentSlot, Slot>> equipment;
    equipment.emplace_back(EquipmentSlot::MAIN_HAND, inventory[36 + heldItemSlot]);
    equipment.emplace_back(EquipmentSlot::HELMET, inventory[5]);
    equipment.emplace_back(EquipmentSlot::CHESTPLATE, inventory[6]);
    equipment.emplace_back(EquipmentSlot::LEGGINGS, inventory[7]);
    equipment.emplace_back(EquipmentSlot::BOOTS, inventory[8]);
    equipment.emplace_back(EquipmentSlot::OFF_HAND, inventory[45]);
    return equipment;
}

void Player::setPosition(const Vector3& position) { this->position = position; }
void Player::setRotation(const Vector2& rotation) { this->rotation = rotation; }
void Player::setHealth(float health) { this->health = health; }
void Player::setFood(int food) { this->food = food; }
void Player::setDead(bool dead) { this->dead = dead; }
void Player::setOnGround(bool onGround) { this->onGround = onGround; }
void Player::setHeldItemSlot(short slot) {
    if (this->heldItemSlot != slot) {
        this->heldItemSlot = slot;
        broadcastEquipmentUpdate();
    }
}
void Player::setInventorySlot(int slot, const Slot& item) {
    if (slot >= 0 && slot < inventory.size()) {
        inventory[slot] = item;
        if (slot == (36 + heldItemSlot) || (slot >= 5 && slot <= 8) || slot == 45) {
            broadcastEquipmentUpdate();
        }
    }
}
void Player::setFlying(bool flying) { this->flying = flying; }
void Player::setViewDistance(uint8_t viewDistance) {
    const uint8_t maxViewDistance = 10;
    this->viewDistance = std::min(viewDistance, maxViewDistance);
}
void Player::setSneaking(bool sneaking) { this->sneaking = sneaking; }
void Player::setSprinting(bool sprinting) { this->sprinting = sprinting; }
void Player::setLocale(const std::string& locale) { this->locale = locale; }
void Player::setChatMode(ChatMode chatMode) { this->chatMode = chatMode; }
void Player::setChatColors(bool chatColors) { this->chatColors = chatColors; }
void Player::setDisplayedSkinParts(uint8_t displayedSkinParts) { this->displayedSkinParts = displayedSkinParts; }
void Player::setMainHand(MainHand mainHand) { this->mainHand = mainHand; }
void Player::setGamemode(Gamemode gamemode) { this->gamemode = gamemode; }
void Player::setAuth(std::unique_ptr<Auth> auth) { this->auth = std::move(auth); }
void Player::setUuid(UUID uuid) { this->uuid = uuid; }
void Player::setSkin(const std::string& skin) { this->skin = skin; }

void Player::broadcastEquipmentUpdate() const {
    auto equipment = getFullEquipment();
    if (!equipment.empty()) {
        for(const auto& equip : equipment) {
            EntityEquipmentPacket packet(id, equip.first, equip.second);
            for (const auto& p : PlayerList::getInstance().getPlayers()) {
                if (p.get() != this) {
                    p->getConnection()->send_packet(packet);
                }
            }
        }
    }
}

void Player::kill() {
    if (dead) return;
    dead = true;

    UpdateHealthPacket healthPacket(0.0f, 0, 0.0f);
    connection->send_packet(healthPacket);

    Metadata meta;
    meta.addFloat(9, 0.0f);
    meta.addPose(6, 2);

    EntityMetadataPacket metadataPacket(id, meta);

    EntityStatusPacket damagePacket(id, static_cast<std::byte>(2));
    EntityStatusPacket deathPacket(id, static_cast<std::byte>(3));

    for (const auto& other : PlayerList::getInstance().getPlayers()) {
        other->getConnection()->send_packet(metadataPacket);
        other->getConnection()->send_packet(damagePacket);
        other->getConnection()->send_packet(deathPacket);
        other->sendChatMessage(nickname + " died!");
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

    inventory.clear();
    inventory.resize(46);

    broadcastEquipmentUpdate();
    LOG_DEBUG("Player " + nickname + " teleported to spawn and healed");
}

void Player::disconnect(std::string reason) const {
    PlayerData::save(*this);

    if (!PlayerList::getInstance().getPlayer(getId())) {
        return;
    }

    PlayerList::getInstance().removePlayer(getId());

    const auto& remainingPlayers = PlayerList::getInstance().getPlayers();

    DestroyEntitiesPacket destroyPacket({getId()});
    for (const auto& client : remainingPlayers) {
        client->sendChatMessage(nickname + " left the game", ChatColor::YELLOW);
        client->getConnection()->send_packet(destroyPacket);
    }

    DisconnectPacketPlay disconnectPacket(reason);
    connection->send_packet(disconnectPacket);

    boost::system::error_code ec;
    connection->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    connection->socket().close(ec);

    LOG_DEBUG("Player " + nickname + " disconnected");
}

void Player::sendChatMessage(const std::string &message, const ChatColor color, const UUID senderUuid) const {
    ChatMessagePacket message_packet = ChatMessagePacket("{\"text\":\"" + message + "\", \"color\":\"" + to_string(color) + "\"}", 1, senderUuid);
    connection->send_packet(message_packet);
}
