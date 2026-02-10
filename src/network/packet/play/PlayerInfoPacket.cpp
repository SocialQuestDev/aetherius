#include "../../../../include/network/packet/play/PlayerInfoPacket.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/network/PacketBuffer.h"

PlayerInfoPacket::PlayerInfoPacket(Action action, std::vector<std::shared_ptr<Player>> players)
    : action(action), players(std::move(players)) {}

void PlayerInfoPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(action);
    buffer.writeVarInt(players.size());

    for (const auto& player : players) {
        buffer.writeUUID(player->getUuid());

        switch (action) {
            case ADD_PLAYER: {
                buffer.writeString(player->getNickname());

                std::string skin = player->getSkin();
                if (skin.empty()) {
                    buffer.writeVarInt(0); // 0 properties
                } else {
                    buffer.writeVarInt(1); // 1 property
                    buffer.writeString("textures");
                    buffer.writeString(skin);
                    buffer.writeBoolean(false); // isSigned = false
                }

                buffer.writeVarInt(1); // gamemode
                buffer.writeVarInt(1); // ping
                buffer.writeBoolean(false); // has display name = false
                break;
            }
            case UPDATE_GAMEMODE:
                buffer.writeVarInt(1); // gamemode
                break;

            case UPDATE_LATENCY:
                buffer.writeVarInt(1); // ping
                break;

            case UPDATE_DISPLAY_NAME:
                buffer.writeBoolean(false); // has display name = false
                break;

            case REMOVE_PLAYER:
                // No other fields are needed for the remove player action
                break;
        }
    }
}
