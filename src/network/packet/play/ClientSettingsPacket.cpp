#include "../../../../include/network/packet/play/ClientSettingsPacket.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/game/player/Player.h"

void ClientSettingsPacket::handle(Connection& connection) {
    auto player = connection.getPlayer();
    if (player) {
        player->setViewDistance(viewDistance);
        // You can also store other settings like locale, chatMode, etc.
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
