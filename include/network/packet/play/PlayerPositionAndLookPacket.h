#pragma once

#include "../InboundPacket.h"
#include "../OutboundPacket.h"

class PlayerPositionAndLookPacket : public InboundPacket, public OutboundPacket {
public:
    // Для исходящего пакета (сервер -> клиент)
    PlayerPositionAndLookPacket(double x, double y, double z, float yaw, float pitch, char flags, int teleportId);

    // Для входящего пакета (клиент -> сервер)
    PlayerPositionAndLookPacket();

    // ID пакета (один для обоих направлений)
    int getPacketId() const override { return 0x28; } // ID для 1.16.5, проверьте для 751 (1.15.2)

    // Методы для OutboundPacket
    void write(PacketBuffer& buffer) override;

    // Методы для InboundPacket
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;

private:
    double x, y, z;
    float yaw, pitch;
    bool onGround; // Только для входящего пакета

    // Только для исходящего пакета
    char flags;
    int teleportId;
};
