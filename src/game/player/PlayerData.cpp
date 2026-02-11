#include "game/player/PlayerData.h"
#include "game/player/Player.h" // Include the full Player definition
#include "network/PacketBuffer.h"
#include "other/Vector2.h"      // Include Vector2 definition
#include "other/Vector3.h"      // Include Vector3 definition
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// Helper function to write a string to the buffer
void writeString(PacketBuffer& buf, const std::string& str) {
    buf.writeVarInt(str.length());
    buf.data.insert(buf.data.end(), str.begin(), str.end());
}

// Helper function to read a string from the buffer
std::string readString(PacketBuffer& buf) {
    int len = buf.readVarInt();
    // FIX: Check readerIndex against the total size of the buffer's data vector.
    if (len > (buf.data.size() - buf.readerIndex)) {
        throw std::runtime_error("Not enough data to read string");
    }
    std::string str(buf.data.begin() + buf.readerIndex, buf.data.begin() + buf.readerIndex + len);
    buf.readerIndex += len;
    return str;
}

void PlayerData::save(const Player& player) {
    fs::create_directory("playerdata");
    UUID uuid = player.getUuid();
    std::string filePath = "playerdata/" + uuid_to_string(uuid.high, uuid.low) + ".dat";

    PacketBuffer buf;

    // Player Attributes
    writeString(buf, player.getNickname());
    writeString(buf, player.getSkin());

    // Position and Rotation
    Vector3 pos = player.getPosition();
    buf.writeDouble(pos.x);
    buf.writeDouble(pos.y);
    buf.writeDouble(pos.z);
    Vector2 rot = player.getRotation();
    buf.writeFloat(rot.x);
    buf.writeFloat(rot.y);

    // Health and Food
    buf.writeFloat(player.getHealth());
    buf.writeInt(player.getFood());

    // State
    buf.writeBoolean(player.isDead());
    buf.writeBoolean(player.isOnGround());
    buf.writeBoolean(player.isFlying());
    buf.writeBoolean(player.isSneaking());
    buf.writeBoolean(player.isSprinting());

    // Inventory
    buf.writeShort(player.getHeldItemSlot());
    const auto& inventory = player.getInventory();
    buf.writeVarInt(inventory.size());
    for (const auto& slot : inventory) {
        buf.writeVarInt(slot.itemId);
        buf.writeByte(slot.count);
    }

    // Settings
    buf.writeByte(player.getViewDistance());
    writeString(buf, player.getLocale());
    buf.writeByte(static_cast<uint8_t>(player.getChatMode()));
    buf.writeBoolean(player.hasChatColors());
    buf.writeByte(player.getDisplayedSkinParts());
    buf.writeByte(static_cast<uint8_t>(player.getMainHand()));
    buf.writeByte(static_cast<uint8_t>(player.getGamemode()));

    // Write to file
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(buf.data.data()), buf.data.size());
    }
}

bool PlayerData::load(Player& player) {
    UUID uuid = player.getUuid();
    std::string filePath = "playerdata/" + uuid_to_string(uuid.high, uuid.low) + ".dat";
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false; // No data file exists for this player.
    }

    std::vector<uint8_t> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    PacketBuffer buf(file_data);

    try {
        // We don't load nickname/skin as they come from the login process.
        readString(buf); // Skip nickname
        readString(buf); // Skip skin

        // Position and Rotation
        double x = buf.readDouble();
        double y = buf.readDouble();
        double z = buf.readDouble();
        player.setPosition({x, y, z});
        float pitch = buf.readFloat();
        float yaw = buf.readFloat();
        player.setRotation({pitch, yaw});

        // Health and Food
        player.setHealth(buf.readFloat());
        player.setFood(buf.readInt());

        // State
        player.setDead(buf.readBoolean());
        player.setOnGround(buf.readBoolean());
        player.setFlying(buf.readBoolean());
        player.setSneaking(buf.readBoolean());
        player.setSprinting(buf.readBoolean());

        // Inventory
        player.setHeldItemSlot(buf.readShort());
        int inventory_size = buf.readVarInt();
        player.getInventory().resize(inventory_size); // Ensure inventory is the correct size before filling
        for (int i = 0; i < inventory_size; ++i) {
            if (buf.readerIndex >= buf.data.size()) break; // Safety check
            int itemId = buf.readVarInt();
            uint8_t count = buf.readByte();
            player.setInventorySlot(i, {itemId, (int)count});
        }

        // Settings
        player.setViewDistance(buf.readByte());
        player.setLocale(readString(buf));
        player.setChatMode(static_cast<ChatMode>(buf.readByte()));
        player.setChatColors(buf.readBoolean());
        player.setDisplayedSkinParts(buf.readByte());
        player.setMainHand(static_cast<MainHand>(buf.readByte()));
        player.setGamemode(static_cast<Gamemode>(buf.readByte()));

    } catch (const std::exception& e) {
        // In case of corrupted data, treat as a new player.
        return false;
    }

    return true;
}
