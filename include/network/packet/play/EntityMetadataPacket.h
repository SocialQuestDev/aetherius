#pragma once
#include "../OutboundPacket.h"
#include <vector>

class Player; // Forward declaration

class EntityMetadataPacket : public OutboundPacket {
public:
    EntityMetadataPacket(const Player& player);
    int getPacketId() const override { return 0x4D; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    uint8_t metadata;
};
