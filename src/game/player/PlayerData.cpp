#include "game/player/PlayerData.h"
#include "game/player/Player.h"
#include "network/PacketBuffer.h"
#include "other/Vector2.h"
#include "other/Vector3.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

void writeString(PacketBuffer& buf, const std::string& str) {
    buf.writeVarInt(str.length());
    if (!str.empty()) {
        buf.data.insert(buf.data.end(), str.begin(), str.end());
    }
}

std::string readString(PacketBuffer& buf) {
    int len = buf.readVarInt();
    if (len < 0) {
        throw std::runtime_error("Invalid string length received");
    }
    if (len == 0) {
        return "";
    }
    if (buf.readerIndex + len > buf.data.size()) {
        throw std::runtime_error("Not enough data to read string of length " + std::to_string(len));
    }
    std::string str(buf.data.begin() + buf.readerIndex, buf.data.begin() + buf.readerIndex + len);
    buf.readerIndex += len;
    return str;
}

void PlayerData::save(const Player& player) {
    fs::create_directory("playerdata");
    UUID uuid = player.getUuid();
    if (uuid.high == 0 && uuid.low == 0) return;

    std::string filePath = "playerdata/" + uuid_to_string(uuid.high, uuid.low) + ".dat";

    PacketBuffer buf;

    writeString(buf, player.getNickname());
    writeString(buf, player.getSkin());

    Vector3 pos = player.getPosition();
    buf.writeDouble(pos.x);
    buf.writeDouble(pos.y);
    buf.writeDouble(pos.z);
    Vector2 rot = player.getRotation();
    buf.writeFloat(rot.x);
    buf.writeFloat(rot.y);

    buf.writeFloat(player.getHealth());
    buf.writeInt(player.getFood());

    buf.writeBoolean(player.isDead());
    buf.writeBoolean(player.isOnGround());
    buf.writeBoolean(player.isFlying());
    buf.writeBoolean(player.isSneaking());
    buf.writeBoolean(player.isSprinting());

    buf.writeShort(player.getHeldItemSlot());
    const auto& inventory = player.getInventory();
    buf.writeVarInt(inventory.size());
    for (const auto& slot : inventory) {
        buf.writeVarInt(slot.itemId);
        buf.writeByte(slot.count);
    }

    buf.writeByte(player.getViewDistance());
    writeString(buf, player.getLocale());
    buf.writeByte(static_cast<uint8_t>(player.getChatMode()));
    buf.writeBoolean(player.hasChatColors());
    buf.writeByte(player.getDisplayedSkinParts());
    buf.writeByte(static_cast<uint8_t>(player.getMainHand()));
    buf.writeByte(static_cast<uint8_t>(player.getGamemode()));

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(buf.data.data()), buf.data.size());
    }
}

bool PlayerData::load(Player& player) {
    UUID uuid = player.getUuid();
    if (uuid.high == 0 && uuid.low == 0) return false;

    std::string filePath = "playerdata/" + uuid_to_string(uuid.high, uuid.low) + ".dat";
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::vector<uint8_t> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    PacketBuffer buf(file_data);

    try {
        readString(buf);
        readString(buf);

        double x = buf.readDouble();
        double y = buf.readDouble();
        double z = buf.readDouble();
        player.setPosition({x, y, z});
        float pitch = buf.readFloat();
        float yaw = buf.readFloat();
        player.setRotation({pitch, yaw});

        player.setHealth(buf.readFloat());
        player.setFood(buf.readInt());

        player.setDead(buf.readBoolean());
        player.setOnGround(buf.readBoolean());
        player.setFlying(buf.readBoolean());
        player.setSneaking(buf.readBoolean());
        player.setSprinting(buf.readBoolean());

        player.setHeldItemSlot(buf.readShort());
        int inventory_size = buf.readVarInt();
        auto& inventory = player.getInventory();
        inventory.assign(inventory_size, {});
        for (int i = 0; i < inventory_size; ++i) {
            if (buf.readerIndex >= buf.data.size()) break;
            int itemId = buf.readVarInt();
            uint8_t count = buf.readByte();
            inventory[i] = {itemId, (int)count};
        }

        player.setViewDistance(buf.readByte());
        player.setLocale(readString(buf));
        player.setChatMode(static_cast<ChatMode>(buf.readByte()));
        player.setChatColors(buf.readBoolean());
        player.setDisplayedSkinParts(buf.readByte());
        player.setMainHand(static_cast<MainHand>(buf.readByte()));
        player.setGamemode(static_cast<Gamemode>(buf.readByte()));

    } catch (const std::exception& e) {
        return false;
    }

    return true;
}
