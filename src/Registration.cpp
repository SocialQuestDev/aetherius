#include "network/PacketRegistry.h"
#include "commands/CommandRegistry.h"

#include "network/packet/inbound/handshake/HandshakePacket.h"
#include "network/packet/inbound/status/StatusRequestPacket.h"
#include "network/packet/inbound/status/StatusPingPacket.h"
#include "network/packet/inbound/login/LoginStartPacket.h"
#include "network/packet/inbound/login/EncryptionResponsePacket.h"
#include "network/packet/inbound/play/KeepAlivePacketPlay.h"
#include "network/packet/inbound/play/TeleportConfirmPacket.h"
#include "network/packet/inbound/play/ClientStatusPacket.h"
#include "network/packet/inbound/play/GetChatMessagePacket.h"
#include "network/packet/inbound/play/PlayerPositionPacket.h"
#include "network/packet/inbound/play/BlockDigRequestPacket.h"
#include "network/packet/inbound/play/HeldItemChangePacket.h"
#include "network/packet/inbound/play/SetCreativeSlotPacket.h"
#include "network/packet/inbound/play/ArmAnimationPacket.h"
#include "network/packet/inbound/play/BlockPlacePacket.h"
#include "network/packet/inbound/play/ClientAbilitiesPacket.h"
#include "network/packet/inbound/play/EntityActionPacket.h"
#include "network/packet/inbound/play/ClientSettingsPacket.h"

#include "commands/gameCommands/PingCommand.h"
#include "commands/gameCommands/KillCommand.h"
#include "commands/gameCommands/HelpGameCommand.h"
#include "commands/gameCommands/TimeCommand.h"
#include "commands/consoleCommands/HelpCommand.h"

void register_all_packets(PacketRegistry& registry) {
    registry.registerPacket(State::HANDSHAKE, 0x00, []{ return std::make_unique<HandshakePacket>(); });

    registry.registerPacket(State::STATUS, 0x00, []{ return std::make_unique<StatusRequestPacket>(); });
    registry.registerPacket(State::STATUS, 0x01, []{ return std::make_unique<StatusPingPacket>(); });

    registry.registerPacket(State::LOGIN, 0x00, []{ return std::make_unique<LoginStartPacket>(); });
    registry.registerPacket(State::LOGIN, 0x01, []{ return std::make_unique<EncryptionResponsePacket>(); });

    registry.registerPacket(State::PLAY, 0x00, []{ return std::make_unique<TeleportConfirmPacket>(); });
    registry.registerPacket(State::PLAY, 0x03, []{ return std::make_unique<GetChatMessagePacket>(); });
    registry.registerPacket(State::PLAY, 0x04, []{ return std::make_unique<ClientStatusPacket>(); });
    registry.registerPacket(State::PLAY, 0x05, []{ return std::make_unique<ClientSettingsPacket>(); });
    registry.registerPacket(State::PLAY, 0x10, []{ return std::make_unique<KeepAlivePacketPlay>(); });
    registry.registerPacket(State::PLAY, 0x12, []{ return std::make_unique<PlayerPositionPacket>(); });
    registry.registerPacket(State::PLAY, 0x13, []{ return std::make_unique<PlayerPositionAndRotationPacket>(); });
    registry.registerPacket(State::PLAY, 0x14, []{ return std::make_unique<PlayerRotationPacket>(); });
    registry.registerPacket(State::PLAY, 0x15, []{ return std::make_unique<PlayerOnGroundPacket>(); });
    registry.registerPacket(State::PLAY, 0x1A, []{ return std::make_unique<ClientAbilitiesPacket>(); });
    registry.registerPacket(State::PLAY, 0x1C, []{ return std::make_unique<EntityActionPacket>(); });
    registry.registerPacket(State::PLAY, 0x1b, []{ return std::make_unique<BlockDigRequestPacket>(); });
    registry.registerPacket(State::PLAY, 0x25, []{ return std::make_unique<HeldItemChangePacket>(); });
    registry.registerPacket(State::PLAY, 0x28, []{ return std::make_unique<SetCreativeSlotPacket>(); });
    registry.registerPacket(State::PLAY, 0x2C, []{ return std::make_unique<ArmAnimationPacket>(); });
    registry.registerPacket(State::PLAY, 0x2E, []{ return std::make_unique<BlockPlacePacket>(); });
}

void register_all_commands(CommandRegistry& registry) {
    registry.registerConsoleCommand(std::make_unique<HelpCommand>());

    registry.registerGameCommand(std::make_unique<PingCommand>());
    registry.registerGameCommand(std::make_unique<KillCommand>());
    registry.registerGameCommand(std::make_unique<TimeCommand>());
    registry.registerGameCommand(std::make_unique<HelpGameCommand>());
}
