#pragma once

#include "game/player/Player.h"
#include "network/packet/OutboundPacket.h"

class World; // Forward declaration

class JoinGamePacket : public OutboundPacket {
public:
    JoinGamePacket(std::shared_ptr<Player> player, World& world);
    int getPacketId() const override { return 0x24; }
    void write(PacketBuffer& buffer) override;

private:
    std::shared_ptr<Player> player;
    World& world;
};
