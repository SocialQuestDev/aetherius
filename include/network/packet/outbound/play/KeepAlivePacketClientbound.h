#pragma once

#include "network/packet/OutboundPacket.h"
class KeepAlivePacketClientbound : public OutboundPacket {
public:
    explicit KeepAlivePacketClientbound(long keepAliveId);
    int getPacketId() const override { return 0x1f; }
    void write(PacketBuffer& buffer) override;

private:
    long keepAliveId;
};
