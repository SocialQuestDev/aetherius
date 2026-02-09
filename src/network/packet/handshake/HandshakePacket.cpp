#include "../../../../include/network/packet/handshake/HandshakePacket.h"
#include "../../../../include/network/Connection.h"

void HandshakePacket::handle(Connection& connection) {
    connection.setState(static_cast<State>(nextState));
}

void HandshakePacket::read(PacketBuffer& buffer) {
    protocolVersion = buffer.readVarInt();
    serverAddress = buffer.readString();
    serverPort = buffer.readUShort();
    nextState = buffer.readVarInt();
}
