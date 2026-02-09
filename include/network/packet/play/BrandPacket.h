#pragma once

#include "../OutboundPacket.h"
#include <string>

class BrandPacket : public OutboundPacket {
public:
    explicit BrandPacket(const std::string& brand);
    int getPacketId() const override;
    void write(PacketBuffer& buffer) override;

private:
    std::string brand;
};
