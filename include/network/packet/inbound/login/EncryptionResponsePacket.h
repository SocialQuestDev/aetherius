#pragma once
#include "network/packet/InboundPacket.h"
#include <vector>
#include <cstdint>

class PacketBuffer;
class Connection;

class EncryptionResponsePacket : public InboundPacket {
public:
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;

    int getPacketId() const override { return 0x01; }

private:
    std::vector<uint8_t> encryptedSharedSecret;
    std::vector<uint8_t> encryptedVerifyToken;
};
