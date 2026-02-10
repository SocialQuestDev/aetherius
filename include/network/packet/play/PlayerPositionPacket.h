#pragma once

#include "../InboundPacket.h"

class PlayerPositionPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x12; }
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;

protected:
    double x, y, z;
    bool onGround;
};

class PlayerPositionAndRotationPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x13; }
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;

protected:
    double x, y, z;
    float yaw, pitch;
    bool onGround;
};

class PlayerRotationPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x14; }
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;

protected:
    float yaw, pitch;
    bool onGround;
};

class PlayerOnGroundPacket : public InboundPacket {
public:
    int getPacketId() const override { return 0x15; }
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;

protected:
    bool onGround;
};
