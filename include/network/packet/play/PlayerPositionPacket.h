#pragma once

#include "../InboundPacket.h"

class PlayerPositionPacket : public InboundPacket {
public:
    virtual void handle(Connection& connection) override = 0;
    void read(PacketBuffer& buffer) override {} // Base read is empty

protected:
    double x, y, z;
    float yaw, pitch;
    bool onGround;
};

class PlayerPositionPacketFull : public PlayerPositionPacket {
public:
    int getPacketId() const override { return 0x12; }
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;
};

class PlayerPositionAndRotationPacket : public PlayerPositionPacket {
public:
    int getPacketId() const override { return 0x13; }
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;
};

class PlayerRotationPacket : public PlayerPositionPacket {
public:
    int getPacketId() const override { return 0x14; }
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;
};

class PlayerOnGroundPacket : public PlayerPositionPacket {
public:
    int getPacketId() const override { return 0x15; }
    void read(PacketBuffer& buffer) override;
    void handle(Connection& connection) override;
};
