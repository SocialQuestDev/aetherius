#pragma once

#include "../PacketBuffer.h"

class Connection; // Forward declaration

class Packet {
public:
    virtual ~Packet() = default;
    virtual int getPacketId() const = 0;
};
