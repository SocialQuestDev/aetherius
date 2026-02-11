#include "network/packet/inbound/status/StatusPingPacket.h"
#include "network/packet/outbound/status/StatusPongPacket.h"
#include "network/Connection.h"

void StatusPingPacket::handle(Connection& connection) {
    StatusPongPacket response(payload);
    connection.send_packet(response);
}

void StatusPingPacket::read(PacketBuffer& buffer) {
    payload = buffer.readLong();
}
