#include "../../../../include/network/packet/play/KeepAlivePacketPlay.h"
#include "../../../../include/network/Connection.h"
#include "../../../../include/console/Logger.h"

void KeepAlivePacketPlay::handle(Connection& connection) {
    // The server should check if the keepAliveId is correct.
    // For now, we'll just log it.
    LOG_DEBUG("Received Keep Alive with ID: " + std::to_string(keepAliveId));
}

void KeepAlivePacketPlay::read(PacketBuffer& buffer) {
    keepAliveId = buffer.readLong();
}
