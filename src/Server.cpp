#include <memory>
#include "../include/Server.h"
#include "../include/Logger.h"
#include "../include/game/player/PlayerList.h"
#include "../include/crypto/RSA.h"
#include "../include/utility/ConfigValidator.h"
#include "../include/game/world/WorldGenerator.h"

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

    const tcp::endpoint endpoint(boost::asio::ip::make_address(ip), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    cur_rsa = rsa::generate();

    if (!public_key) {
        std::vector<uint8_t> tempPublicKey = rsa::get_public_key(cur_rsa);
        public_key = std::make_unique<std::vector<uint8_t>>(tempPublicKey);
    }

    LOG_INFO("Server started on " + ip + ":" + std::to_string(port));
    start_accept();
}

Server& Server::get_instance() {
    return *instance;
}

toml::table& Server::get_config() {
    return config;
}

World& Server::get_world() {
    return *world;
}

std::vector<uint8_t>& Server::get_public_key() const {
    return *public_key;
}

std::vector<uint8_t> Server::decrypt_rsa(const std::vector<uint8_t>& data) const {
    if (!cur_rsa)
        throw std::runtime_error("RSA is not initialized");

    return rsa::decrypt(cur_rsa, data);
}

void Server::start_accept() {
    std::shared_ptr<Connection> new_connection = Connection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
        [this, new_connection](const boost::system::error_code& error) {
            handle_accept(new_connection, error);
        });
}

void Server::handle_accept(const std::shared_ptr<Connection> &new_connection, const boost::system::error_code& error) {
    if (!error) {
        new_connection->start();
    } else {
        LOG_ERROR("Accept error: " + error.message());
    }

    start_accept();
}