#include "network/packet/inbound/play/ClientSettingsPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/EntityMetadataPacket.h"
#include "network/Metadata.h"

void ClientSettingsPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setLocale(locale);
        player->setViewDistance(viewDistance);
        player->setChatMode(static_cast<ChatMode>(chatMode));
        player->setChatColors(chatColors);
        player->setDisplayedSkinParts(displayedSkinParts);
        player->setMainHand(static_cast<MainHand>(mainHand));

        Metadata metadata;

        uint8_t statusFlags = 0;
        if (player->isSneaking()) statusFlags |= 0x02;
        if (player->isSprinting()) statusFlags |= 0x08;
        metadata.addByte(0, statusFlags);

        metadata.addPose(6, player->isSneaking() ? 5 : 0);

        metadata.addByte(19, static_cast<uint8_t>(player->getMainHand()));

        metadata.addByte(16, displayedSkinParts);

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
