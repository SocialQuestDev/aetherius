#pragma once
#include "../PacketBuffer.h"

class IOutPacket {
public:
    virtual void serialize(PacketBuffer& writer) = 0;

    virtual ~IOutPacket() {}
};