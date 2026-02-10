#include "../../../../include/network/packet/play/PlayerInfoPacket.h"
#include "../../../../include/game/player/Player.h"
#include "../../../../include/network/PacketBuffer.h"

PlayerInfoPacket::PlayerInfoPacket(Action action, const std::vector<std::shared_ptr<Player>>& players)
    : action(action), players(players) {}

void PlayerInfoPacket::write(PacketBuffer& buffer) {
    buffer.writeVarInt(static_cast<int>(action));
    buffer.writeVarInt(players.size()); // Number of players in the data array

    for (const auto& player : players) {
        buffer.writeUUID(player->getUuid());

        switch (action) {
            case ADD_PLAYER:
                buffer.writeString(player->getNickname());

                // Properties (for skin)
                buffer.writeVarInt(1); // properties length
                buffer.writeString("textures");
                buffer.writeString(player->getSkin());
                buffer.writeBoolean(false); // is signed

                buffer.writeVarInt(1); // gamemode: creative
                buffer.writeVarInt(100); // ping
                buffer.writeBoolean(false); // has display name
                break;

            case UPDATE_GAMEMODE:
                buffer.writeVarInt(1); // gamemode
                break;

            case UPDATE_LATENCY:
                buffer.writeVarInt(100); // ping
                break;

            case UPDATE_DISPLAY_NAME:
                buffer.writeBoolean(false); // has display name
                break;

            case REMOVE_PLAYER:
                // No other fields are needed for the remove player action
                break;
        }
    }
}
