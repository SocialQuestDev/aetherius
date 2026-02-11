#include "network/packet/inbound/play/ClientSettingsPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/EntityMetadataPacket.h"
#include "network/Metadata.h"

void ClientSettingsPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        // Update the player object with the new settings
        player->setLocale(locale);
        player->setViewDistance(viewDistance);
        player->setChatMode(static_cast<ChatMode>(chatMode));
        player->setChatColors(chatColors);
        player->setDisplayedSkinParts(displayedSkinParts);
        player->setMainHand(static_cast<MainHand>(mainHand));

        // Broadcast a comprehensive visual state update to all players
        Metadata metadata;

        // Add current status (sneaking, sprinting, etc.)
        uint8_t statusFlags = 0;
        if (player->isSneaking()) statusFlags |= 0x02;
        if (player->isSprinting()) statusFlags |= 0x08;
        metadata.addByte(0, statusFlags);

        // Add current pose
        metadata.addPose(6, player->isSneaking() ? 5 : 0);

        // Add the updated main hand
        metadata.addByte(19, static_cast<uint8_t>(player->getMainHand()));

        // Add the updated skin parts
        metadata.addByte(16, displayedSkinParts);

        // Send the complete metadata packet to everyone, including the sender
        EntityMetadataPacket metadataPacket(player->getId(), metadata);
        for (const auto& p : PlayerList::getInstance().getPlayers()) {
            p->getConnection()->send_packet(metadataPacket);
        }
    }
}

void ClientSettingsPacket::read(PacketBuffer& buffer) {
    locale = buffer.readString();
    viewDistance = buffer.readByte();
    chatMode = buffer.readVarInt();
    chatColors = buffer.readBoolean();
    displayedSkinParts = buffer.readByte();
    mainHand = buffer.readVarInt();
}
