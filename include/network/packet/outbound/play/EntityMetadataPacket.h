#pragma once

#include "network/packet/OutboundPacket.h"
#include "network/Metadata.h"

class EntityMetadataPacket : public OutboundPacket {
public:
    EntityMetadataPacket(int entityId, const Metadata& metadata);
    int getPacketId() const override { return 0x44; }
    void write(PacketBuffer& buffer) override;

private:
    int entityId;
    const Metadata& metadata;
};
