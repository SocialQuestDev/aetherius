#pragma once

#include "network/PacketBuffer.h"

class Connection;

class Packet {
public:
    virtual ~Packet() = default;
    virtual int getPacketId() const = 0;
};
