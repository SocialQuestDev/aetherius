#pragma once
#include "../PacketBuffer.h"

class IInPacket {
public:
    virtual void deserialize(PacketBuffer& reader) = 0;

    virtual ~IInPacket() {}
};