#pragma once

#include "Packet.h"

class OutboundPacket : public Packet {
public:
    virtual void write(PacketBuffer& buffer) = 0;
};
