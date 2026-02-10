#pragma once
#include "../InboundPacket.h"

class EntityActionPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x1C; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    int entityId;
    int actionId;
    int jumpBoost;
};
