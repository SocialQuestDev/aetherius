#pragma once

#include "../OutboundPacket.h"
#include "../../../auth/UUID.h"
#include <string>

class LoginSuccessPacket : public OutboundPacket {
public:
    LoginSuccessPacket(const UUID& uuid, const std::string& nickname);
    int getPacketId() const override { return 0x02; }
    void write(PacketBuffer& buffer) override;

private:
    UUID uuid;
    std::string nickname;
};
