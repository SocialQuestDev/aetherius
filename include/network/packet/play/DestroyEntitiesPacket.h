#pragma once
#include <vector>
#include "../OutboundPacket.h"

class PacketBuffer;

class DestroyEntitiesPacket : public OutboundPacket {
public:
    explicit DestroyEntitiesPacket(const std::vector<int> &entities);
    int getPacketId() const override { return 0x36; }
    void write(PacketBuffer& buffer) override;

private:
    std::vector<int> entityIds;
};
