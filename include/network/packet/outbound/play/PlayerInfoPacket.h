#pragma once
#include "network/packet/OutboundPacket.h"
#include <vector>
#include <memory>

class Player;

class PlayerInfoPacket : public OutboundPacket {
public:
    enum Action {
        ADD_PLAYER,
        UPDATE_GAMEMODE,
        UPDATE_LATENCY,
        UPDATE_DISPLAY_NAME,
        REMOVE_PLAYER
    };

    PlayerInfoPacket(Action action, std::vector<std::shared_ptr<Player>> players);
    int getPacketId() const override { return 0x32; }
    void write(PacketBuffer& buffer) override;

private:
    Action action;
    std::vector<std::shared_ptr<Player>> players;
};
