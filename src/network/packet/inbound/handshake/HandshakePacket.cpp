#include "network/packet/inbound/handshake/HandshakePacket.h"
#include "network/Connection.h"

void HandshakePacket::handle(Connection& connection) {
    if (protocolVersion <= 754 || protocolVersion >= 751) connection.set_protocol_version(protocolVersion);
    else connection.set_protocol_version(751);
    connection.setState(static_cast<State>(nextState));
}

void HandshakePacket::read(PacketBuffer& buffer) {
    protocolVersion = buffer.readVarInt();
    serverAddress = buffer.readString();
    serverPort = buffer.readUShort();
    nextState = buffer.readVarInt();
}
