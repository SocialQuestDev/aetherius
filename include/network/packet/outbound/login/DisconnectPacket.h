#pragma once

#include "network/packet/OutboundPacket.h"
#include <string>

class DisconnectPacket : public OutboundPacket {
public:
    explicit DisconnectPacket(const std::string& reason);
    void write(PacketBuffer& buffer) override;
    int getPacketId() const override;

private:
    std::string reason_;
};
