#pragma once

#include <map>
#include <functional>
#include <memory>
#include "network/Connection.h"
#include "packet/InboundPacket.h"

using InboundPacketCreator = std::function<std::unique_ptr<InboundPacket>()>;

class PacketRegistry {
public:
    void registerPacket(State state, int packetId, InboundPacketCreator creator);
    std::unique_ptr<InboundPacket> createPacket(State state, int packetId);

private:
    std::map<State, std::map<int, InboundPacketCreator>> inboundPacketCreators;
};
