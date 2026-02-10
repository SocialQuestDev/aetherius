#include "../../../../include/network/packet/play/DeclareCommandsPacket.h"

DeclareCommandsPacket::DeclareCommandsPacket(const CommandRegistry& registry) : registry_(registry) {}

void DeclareCommandsPacket::write(PacketBuffer &buffer) {
    const auto& commands = registry_.get_game_commands();

    // Number of nodes
    buffer.writeVarInt(commands.size() + 1); // +1 for the root node

    // Root node
    buffer.writeByte(0x00); // Flags: root node
    buffer.writeVarInt(commands.size()); // Number of children
    for (int i = 0; i < commands.size(); ++i) {
        buffer.writeVarInt(i + 1); // Child index
    }

    // Command nodes
    for (const auto& command : commands) {
        buffer.writeByte(0x01 | 0x04); // Flags: literal, executable
        buffer.writeVarInt(0); // No children
        buffer.writeString(command.first);
    }

    // Root node index
    buffer.writeVarInt(0);
}
