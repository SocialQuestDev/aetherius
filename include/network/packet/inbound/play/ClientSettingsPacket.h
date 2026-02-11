#pragma once
#include "network/packet/InboundPacket.h"
#include <string>

class ClientSettingsPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x05; }
    void handle(Connection& connection) override;
    void read(PacketBuffer& buffer) override;

private:
    std::string locale;
    uint8_t viewDistance;
    int chatMode;
    bool chatColors;
    uint8_t displayedSkinParts;
    int mainHand;
};
