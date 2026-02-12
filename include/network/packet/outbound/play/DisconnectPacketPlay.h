#pragma once

#include "network/packet/OutboundPacket.h"
#include <string>

class DisconnectPacketPlay : public OutboundPacket {
public:
    explicit DisconnectPacketPlay(const std::string& reason);
    void write(PacketBuffer& buffer) override;
    int getPacketId() const override;

private:
    std::string reason_;
};
