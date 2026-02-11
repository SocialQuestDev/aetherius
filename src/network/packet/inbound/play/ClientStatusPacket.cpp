#include "network/packet/inbound/play/ClientStatusPacket.h"
#include "network/Connection.h"
#include "Server.h"
#include "console/Logger.h"
#include "game/world/World.h"
#include "game/player/Player.h"

void ClientStatusPacket::handle(Connection& connection) {
    if (actionId == 0) { // Perform respawn
        LOG_DEBUG("Player requested respawn.");

        Server& server = Server::get_instance();

        // packet_respawn: 0x3A
        PacketBuffer rb;
        rb.writeVarInt(0x39);
        rb.writeNbt(server.get_world().getDimension());
        rb.writeString("minecraft:overworld");
        rb.writeLong(0);
        rb.writeByte(1);
        rb.writeByte(255);
        rb.writeBoolean(false);
        rb.writeBoolean(false);
        rb.writeBoolean(true);
        connection.send_packet(rb);

        connection.getPlayer()->teleportToSpawn();
    }
}

void ClientStatusPacket::read(PacketBuffer& buffer) {
    actionId = buffer.readVarInt();
}
