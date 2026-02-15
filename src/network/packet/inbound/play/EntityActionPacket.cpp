#include "network/packet/inbound/play/EntityActionPacket.h"
#include "network/Connection.h"
#include "game/player/Player.h"
#include "network/packet/outbound/play/EntityMetadataPacket.h"
#include "game/player/PlayerList.h"
#include "console/Logger.h"
#include "network/Metadata.h"
#include "Server.h"

void EntityActionPacket::handle(Connection& connection) {
    auto self = connection.shared_from_this();
    const int action = actionId;
    Server::get_instance().post_game_task([self, action]() {
        auto player = self->getPlayer();
        if (!player) return;

        bool metadataChanged = false;
        switch (action) {
            case 0: // Start sneaking
                player->setSneaking(true);
                metadataChanged = true;
                break;
            case 1: // Stop sneaking
                player->setSneaking(false);
                metadataChanged = true;
                break;
            case 3: // Start sprinting
                player->setSprinting(true);
                metadataChanged = true;
                break;
            case 4: // Stop sprinting
                player->setSprinting(false);
                metadataChanged = true;
                break;
            // Other actions (leaving bed, etc.) can be handled here.
        }

        if (metadataChanged) {
            Metadata metadata;
            uint8_t statusFlags = 0;
            if (player->isSneaking()) statusFlags |= 0x02;
            if (player->isSprinting()) statusFlags |= 0x08;

            metadata.addByte(0, statusFlags); // Index 0: Status flags
            metadata.addPose(6, player->isSneaking() ? 5 : 0); // Index 6: Pose

            EntityMetadataPacket metadataPacket(player->getId(), metadata);
            for (const auto& other : PlayerList::getInstance().getPlayers()) {
                if (other->getId() != player->getId()) {
                    other->getConnection()->send_packet(metadataPacket);
                }
            }
        }
    });
}

void EntityActionPacket::read(PacketBuffer& buffer) {
    entityId = buffer.readVarInt();
    actionId = buffer.readVarInt();
    jumpBoost = buffer.readVarInt();
}
