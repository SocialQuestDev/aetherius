#pragma once

#include "network/packet/OutboundPacket.h"
#include <memory>

class Player;

class SpawnNamedEntityPacket : public OutboundPacket {
public:
    explicit SpawnNamedEntityPacket(const std::shared_ptr<Player>& player);

    int getPacketId() const override { return 0x04; }
    void write(PacketBuffer& buffer) override;

private:
    std::shared_ptr<Player> player;
};
