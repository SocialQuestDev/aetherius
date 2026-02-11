#pragma once

#include "network/PacketBuffer.h"

class Connection; // Forward declaration

class Packet {
public:
    virtual ~Packet() = default;
    virtual int getPacketId() const = 0;
};
