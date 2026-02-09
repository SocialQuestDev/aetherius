#pragma once

#include "Packet.h"

class Connection;

class InboundPacket : public Packet {
public:
    virtual void handle(Connection& connection) = 0;
    virtual void read(PacketBuffer& buffer) = 0;
};
