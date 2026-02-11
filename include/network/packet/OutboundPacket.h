#pragma once

#include "network/packet/Packet.h"

class OutboundPacket : public Packet {
public:
    virtual void write(PacketBuffer& buffer) = 0;
};
