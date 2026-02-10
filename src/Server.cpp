#include <memory>
#include "../include/Server.h"
#include "../include/commands/CommandRegistry.h"
#include "../include/console/Logger.h"
#include "../include/game/player/PlayerList.h"
#include "../include/crypto/RSA.h"
#include "../include/utility/ConfigValidator.h"
#include "../include/game/world/WorldGenerator.h"
#include "../include/network/packet/handshake/HandshakePacket.h"
#include "../include/network/packet/status/StatusRequestPacket.h"
#include "../include/network/packet/status/StatusPingPacket.h"
#include "../include/network/packet/login/LoginStartPacket.h"
#include "../include/network/packet/login/EncryptionResponsePacket.h"
#include "../include/network/packet/play/KeepAlivePacketPlay.h"
#include "../include/network/packet/play/TeleportConfirmPacket.h"
#include "../include/network/packet/play/ClientStatusPacket.h"
#include "../include/network/packet/play/GetChatMessagePacket.h"
#include "../include/network/packet/play/PlayerPositionPacket.h"
#include "../include/network/packet/play/BlockDigRequestPacket.h"
#include "../include/network/packet/play/HeldItemChangePacket.h"
#include "../include/network/packet/play/SetCreativeSlotPacket.h"
#include "../include/network/packet/play/ArmAnimationPacket.h"
#include "../include/network/packet/play/BlockPlacePacket.h"
#include "../include/network/packet/play/ClientAbilitiesPacket.h"
#include "../include/network/packet/play/EntityActionPacket.h"
#include "../include/network/packet/play/ClientSettingsPacket.h"
#include "../include/commands/gameCommands/PingCommand.h"
#include "../include/commands/gameCommands/KillCommand.h"


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
    // ФИКС: Принудительно используем IPv4, чтобы избежать таймаутов на Windows/Linux
    boost::system::error_code ec;
    auto address = boost::asio::ip::make_address(ip, ec);

    if (ec) {
        LOG_WARN("Invalid IP address in config: " + ip + ". Fallback to 0.0.0.0");
        address = boost::asio::ip::make_address("0.0.0.0");
    }

    // Создаем endpoint (точку подключения)
    const tcp::endpoint endpoint(address, port);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    cur_rsa = rsa::generate();

    if (!public_key) {
        std::vector<uint8_t> tempPublicKey = rsa::get_public_key(cur_rsa);
        public_key = std::make_unique<std::vector<uint8_t>>(tempPublicKey);
    }

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
    return *public_key;
}

std::vector<uint8_t> Server::decrypt_rsa(const std::vector<uint8_t>& data) const {
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
    command_registry_.registerGameCommand(std::make_unique<PingCommand>());
    command_registry_.registerGameCommand(std::make_unique<KillCommand>());
}

void Server::start_accept() {
    // Создаем "пустое" соединение, которое будет ждать клиента
    std::shared_ptr<Connection> new_connection = Connection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
        [this, new_connection](const boost::system::error_code& error) {
            handle_accept(new_connection, error);
        });
}

void Server::handle_accept(const std::shared_ptr<Connection> &new_connection, const boost::system::error_code& error) {
    if (!error) {
        std::string client_ip = "unknown";
        try {
            client_ip = new_connection->socket().remote_endpoint().address().to_string();
        } catch(...) {}

        LOG_INFO("New connection from: " + client_ip);

        // === ГЛАВНЫЙ ФИКС ЛАГОВ ===
        // Отключаем алгоритм Нагла. Без этого MOTD тормозит.
        boost::system::error_code ec;
        new_connection->socket().set_option(boost::asio::ip::tcp::no_delay(true), ec);
        
        // Запускаем чтение в Connection
        new_connection->start();
    } else {
        LOG_ERROR("Accept error: " + error.message());
    }

    // Сразу готовимся принимать следующего, не дожидаясь обработки текущего
    start_accept();
}
