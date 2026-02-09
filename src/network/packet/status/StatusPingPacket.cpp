#include "../../../../include/network/packet/status/StatusPingPacket.h"
#include "../../../../include/network/packet/status/StatusPongPacket.h"
#include "../../../../include/network/Connection.h"

void StatusPingPacket::handle(Connection& connection) {
    StatusPongPacket response(payload);
    connection.send_packet(response);
}

void StatusPingPacket::read(PacketBuffer& buffer) {
    payload = buffer.readLong();
}
