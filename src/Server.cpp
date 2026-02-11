#include <memory>
#include "Server.h"
#include "commands/CommandRegistry.h"
#include "console/Logger.h"
#include "game/player/PlayerList.h"
#include "crypto/RSA.h"
#include "utility/ConfigValidator.h"
#include "game/world/WorldGenerator.h"
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
#include "commands/consoleCommands/HelpCommand.h"

Server* Server::instance;

Server::Server(boost::asio::io_context& io_context) : acceptor_(io_context), io_context_(io_context) {
    instance = this;

    LOG_INFO("Loading configuration...");
    config = ConfigValidator::load_and_validate("config.toml");
    LOG_INFO("Configuration loaded and validated.");

    world = std::make_unique<World>(std::make_unique<FlatWorldGenerator>());

    const int port = config["server"]["port"].value_or(25565);
    const std::string ip = config["server"]["ip"].value_or("0.0.0.0");

    if (!config["server"]["online_mode"].value_or(false)) {
        LOG_WARN("SERVER IS RUNNING IN OFFLINE/INSECURE MODE!");
        LOG_WARN("The server will make no attempt to authenticate usernames. Beware.");
        LOG_WARN("While this makes the game possible to play without internet access, it also opens up the ability for hackers to connect with any username they choose.");
        LOG_WARN("To change this, set 'online-mode' to 'true' in the config.toml file.");
    }

    boost::system::error_code ec;
    auto address = boost::asio::ip::make_address(ip, ec);

    if (ec) {
        LOG_WARN("Invalid IP address in config: " + ip + ". Fallback to 0.0.0.0");
        address = boost::asio::ip::make_address("0.0.0.0");
    }

    const tcp::endpoint endpoint(address, port);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    cur_rsa = nullptr;
    boost::asio::post(io_context_, [this]() { init_rsa_async(); });

    register_packets();
    register_commands();

    console_manager_ = std::make_unique<ConsoleManager>(io_context_);

    LOG_INFO("Server started on " + ip + ":" + std::to_string(port));
    start_accept();
}

void Server::start_console() {
    console_manager_->start();
}

Server& Server::get_instance() {
    return *instance;
}

CommandRegistry& Server::get_command_registry() {
    return command_registry_;
}

toml::table& Server::get_config() {
    return config;
}

toml::table& Server::get_world_config() {
    return *config["world"].as_table();
}

World& Server::get_world() {
    return *world;
}

PacketRegistry& Server::get_packet_registry() {
    return packet_registry_;
}

std::vector<uint8_t>& Server::get_public_key() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(rsa_mutex_));
    if (!rsa_initialized_)
        throw std::runtime_error("RSA is not yet initialized");
    return *public_key;
}

std::vector<uint8_t> Server::decrypt_rsa(const std::vector<uint8_t>& data) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(rsa_mutex_));
    if (!cur_rsa)
        throw std::runtime_error("RSA is not initialized");
    return rsa::decrypt(cur_rsa, data);
}

void Server::register_packets() {
    packet_registry_.registerPacket(State::HANDSHAKE, 0x00, []{ return std::make_unique<HandshakePacket>(); });

    packet_registry_.registerPacket(State::STATUS, 0x00, []{ return std::make_unique<StatusRequestPacket>(); });
    packet_registry_.registerPacket(State::STATUS, 0x01, []{ return std::make_unique<StatusPingPacket>(); });

    packet_registry_.registerPacket(State::LOGIN, 0x00, []{ return std::make_unique<LoginStartPacket>(); });
    packet_registry_.registerPacket(State::LOGIN, 0x01, []{ return std::make_unique<EncryptionResponsePacket>(); });

    packet_registry_.registerPacket(State::PLAY, 0x00, []{ return std::make_unique<TeleportConfirmPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x03, []{ return std::make_unique<GetChatMessagePacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x04, []{ return std::make_unique<ClientStatusPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x05, []{ return std::make_unique<ClientSettingsPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x10, []{ return std::make_unique<KeepAlivePacketPlay>(); });
    packet_registry_.registerPacket(State::PLAY, 0x12, []{ return std::make_unique<PlayerPositionPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x13, []{ return std::make_unique<PlayerPositionAndRotationPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x14, []{ return std::make_unique<PlayerRotationPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x15, []{ return std::make_unique<PlayerOnGroundPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x1A, []{ return std::make_unique<ClientAbilitiesPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x1C, []{ return std::make_unique<EntityActionPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x1b, []{ return std::make_unique<BlockDigRequestPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x25, []{ return std::make_unique<HeldItemChangePacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x28, []{ return std::make_unique<SetCreativeSlotPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x2C, []{ return std::make_unique<ArmAnimationPacket>(); });
    packet_registry_.registerPacket(State::PLAY, 0x2E, []{ return std::make_unique<BlockPlacePacket>(); });
}

void Server::register_commands() {
    // Console Commands
    command_registry_.registerConsoleCommand(std::make_unique<HelpCommand>());

    // Game Commands
    command_registry_.registerGameCommand(std::make_unique<PingCommand>());
    command_registry_.registerGameCommand(std::make_unique<KillCommand>());
    command_registry_.registerGameCommand(std::make_unique<HelpGameCommand>());
}

void Server::start_accept() {
    std::shared_ptr<Connection> new_connection = Connection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
        [this, new_connection](const boost::system::error_code& error) {
            handle_accept(new_connection, error);
        });
}

void Server::init_rsa_async() {
    auto rsa = rsa::generate();
    std::vector<uint8_t> tempPublicKey = rsa::get_public_key(rsa);

    std::lock_guard<std::mutex> lock(rsa_mutex_);
    cur_rsa = rsa;
    public_key = std::make_unique<std::vector<uint8_t>>(tempPublicKey);
    rsa_initialized_ = true;
    LOG_INFO("RSA encryption initialized");
}

void Server::handle_accept(const std::shared_ptr<Connection> &new_connection, const boost::system::error_code& error) {
    if (!error) {
        std::string client_ip = "unknown";
        try {
            client_ip = new_connection->socket().remote_endpoint().address().to_string();
        } catch(...) {}

        LOG_INFO("New connection from: " + client_ip);
        boost::system::error_code ec;
        new_connection->socket().set_option(boost::asio::ip::tcp::no_delay(true), ec);
        new_connection->start();
    } else {
        LOG_ERROR("Accept error: " + error.message());
    }
    start_accept();
}
