#include "network/packet/inbound/play/ClientSettingsPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "network/packet/outbound/play/EntityMetadataPacket.h"
#include "network/Metadata.h"
#include "Server.h"

void ClientSettingsPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const std::string locale_copy = locale;
    const uint8_t view_dist = viewDistance;
    const int chat_mode = chatMode;
    const bool chat_colors = chatColors;
    const uint8_t skin_parts = displayedSkinParts;
    const int main_hand = mainHand;
    Server::get_instance().post_game_task([self, locale_copy, view_dist, chat_mode, chat_colors, skin_parts, main_hand]() {
        auto player = self->getPlayer();
        if (player) {
            player->setLocale(locale_copy);
            player->setViewDistance(view_dist);
            player->setChatMode(static_cast<ChatMode>(chat_mode));
            player->setChatColors(chat_colors);
            player->setDisplayedSkinParts(skin_parts);
            player->setMainHand(static_cast<MainHand>(main_hand));

            Metadata metadata;

            uint8_t statusFlags = 0;
            if (player->isSneaking()) statusFlags |= 0x02;
            if (player->isSprinting()) statusFlags |= 0x08;
            metadata.addByte(0, statusFlags);

            metadata.addPose(6, player->isSneaking() ? 5 : 0);

            metadata.addByte(19, static_cast<uint8_t>(player->getMainHand()));

            metadata.addByte(16, skin_parts);

            EntityMetadataPacket metadataPacket(player->getId(), metadata);
            for (const auto& p : PlayerList::getInstance().getPlayers()) {
                p->getConnection()->send_packet(metadataPacket);
            }
        }
    });
}

void ClientSettingsPacket::read(PacketBuffer& buffer) {
    locale = buffer.readString();
    viewDistance = buffer.readByte();
    chatMode = buffer.readVarInt();
    chatColors = buffer.readBoolean();
    displayedSkinParts = buffer.readByte();
    mainHand = buffer.readVarInt();
}
