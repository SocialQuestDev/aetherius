#pragma once

#include "../Packet.h"
#include "../../../commands/CommandRegistry.h"

class DeclareCommandsPacket : public OutboundPacket {
public:
    explicit DeclareCommandsPacket(const CommandRegistry& registry);
    void write(PacketBuffer &buffer) override;
    int getPacketId() const override { return 0x12; }

private:
    const CommandRegistry& registry_;
};
