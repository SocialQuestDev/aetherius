#include <memory>
#include <atomic>
#include "Server.h"
#include "commands/CommandRegistry.h"
#include "console/Logger.h"
#include "game/player/PlayerList.h"
#include "crypto/RSA.h"
#include "utility/ConfigValidator.h"
#include "game/world/WorldGenerator.h"
#include "network/PacketRegistry.h"
#include "commands/CommandRegistry.h"

Server* Server::instance;

void register_all_packets(PacketRegistry& registry);
void register_all_commands(CommandRegistry& registry);

Server::Server(boost::asio::io_context& io_context)
    : acceptor_(io_context), io_context_(io_context), tick_timer_(io_context) {
    instance = this;

    LOG_INFO("Loading configuration...");
    config = ConfigValidator::load_and_validate("config.toml");
    LOG_INFO("Configuration loaded and validated.");

    const std::string worldName = config["world"]["name"].value_or("world");
    world = std::make_unique<World>(std::make_unique<FlatWorldGenerator>(), worldName);

    const int port = config["server"]["port"].value_or(25565);
    const std::string ip = config["server"]["ip"].value_or("0.0.0.0");
    LOG_INFO("Starting server on " + ip + ":" + std::to_string(port));

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

    register_all_packets(packet_registry_);
    register_all_commands(command_registry_);

    console_manager_ = std::make_unique<ConsoleManager>(io_context_);

    pre_generate_world();

    LOG_INFO("Waiting for players...");
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

boost::asio::io_context& Server::get_io_context() {
    return io_context_;
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

void Server::pre_generate_world() {
    LOG_INFO("Pre-generating world...");
    const int radius = 8;
    const int total_chunks = (2 * radius) * (2 * radius);
    auto chunks_generated = std::make_shared<std::atomic<int>>(0);

    for (int x = -radius; x < radius; ++x) {
        for (int z = -radius; z < radius; ++z) {
            world->getOrGenerateChunk(x, z, [chunks_generated, total_chunks](ChunkColumn* chunk) {
                if (chunk) {
                    int generated_count = ++(*chunks_generated);
                    if (generated_count == total_chunks) {
                        LOG_INFO("World pre-generation complete.");
                    }
                }
            });
        }
    }
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

void Server::start_tick_system() {
    tick_timer_.expires_after(std::chrono::milliseconds(50)); // 20 ticks per second
    tick_timer_.async_wait([this](const boost::system::error_code& ec) {
        if (!ec) {
            tick();
            start_tick_system();
        }
    });
}

void Server::tick() {
    world->tick();
}
