#include "network/packet/outbound/play/DeclareCommandsPacket.h"

DeclareCommandsPacket::DeclareCommandsPacket(const CommandRegistry& registry) : registry_(registry) {}

void DeclareCommandsPacket::write(PacketBuffer &buffer) {
    const auto& commands = registry_.get_game_commands();

    // Node list
    std::vector<std::vector<uint8_t>> nodes;
    std::vector<int> root_children;

    // Root node
    int node_index = 0;
    std::vector<uint8_t> root_node_data;
    root_node_data.push_back(0x00); // Flags: root
    nodes.push_back(root_node_data);

    for (const auto& [name, command] : commands) {
        root_children.push_back(++node_index);
        std::vector<uint8_t> command_node_data;

        if (name == "kill") {
            command_node_data.push_back(0x01); // Flags: literal
            command_node_data.push_back(1); // One child
            PacketBuffer temp_buf;
            temp_buf.writeVarInt(node_index + 1);
            command_node_data.insert(command_node_data.end(), temp_buf.data.begin(), temp_buf.data.end());

            PacketBuffer temp_buf_str;
            temp_buf_str.writeString(name);
            command_node_data.insert(command_node_data.end(), temp_buf_str.data.begin(), temp_buf_str.data.end());
            nodes.push_back(command_node_data);

            node_index++;
            std::vector<uint8_t> arg_node_data;
            arg_node_data.push_back(0x02 | 0x04); // Flags: argument, executable
            arg_node_data.push_back(0); // No children

            PacketBuffer temp_buf_arg;
            temp_buf_arg.writeString("target");
            temp_buf_arg.writeString("minecraft:game_profile");
            arg_node_data.insert(arg_node_data.end(), temp_buf_arg.data.begin(), temp_buf_arg.data.end());
            nodes.push_back(arg_node_data);

        } else {
            command_node_data.push_back(0x01 | 0x04); // Flags: literal, executable
            command_node_data.push_back(0); // No children
            PacketBuffer temp_buf;
            temp_buf.writeString(name);
            command_node_data.insert(command_node_data.end(), temp_buf.data.begin(), temp_buf.data.end());
            nodes.push_back(command_node_data);
        }
    }

    PacketBuffer temp_buf;
    temp_buf.writeVarInt(root_children.size());
    for (int child : root_children) {
        temp_buf.writeVarInt(child);
    }
    nodes[0].insert(nodes[0].end(), temp_buf.data.begin(), temp_buf.data.end());

    buffer.writeVarInt(nodes.size());
    for (const auto& node : nodes) {
        for (const auto& node_data : node) {
            buffer.writeByte(node_data);
        }
    }
    buffer.writeVarInt(0); // Root node index
}
