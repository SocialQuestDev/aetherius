#include "../../include/network/PacketRegistry.h"
#include "../../include/network/packet/InboundPacket.h"

void PacketRegistry::registerPacket(State state, int packetId, InboundPacketCreator creator) {
    inboundPacketCreators[state][packetId] = std::move(creator);
}

std::unique_ptr<InboundPacket> PacketRegistry::createPacket(State state, int packetId) {
    if (inboundPacketCreators.count(state) && inboundPacketCreators[state].count(packetId)) {
        return inboundPacketCreators[state][packetId]();
    }
    return nullptr;
}
