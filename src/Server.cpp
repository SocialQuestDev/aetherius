#include <memory>
#include "../include/Server.h"
#include "../include/Logger.h"
#include "../include/game/player/PlayerList.h"
#include "../include/crypto/RSA.h"

Server* Server::instance;

Server::Server(boost::asio::io_context& io_context, short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    instance = this;

    LOG_INFO("Loading configuration");
    config = toml::parse_file("config.toml");
    LOG_INFO("Configuration loaded");

    if (!cur_rsa)
        cur_rsa = rsa::generate();

    if (!public_key) {
        std::vector<uint8_t> tempPublicKey = rsa::get_public_key(cur_rsa);

        public_key = std::make_unique<std::vector<uint8_t>>(tempPublicKey);
    }

    LOG_INFO("Server starting on port " + std::to_string(port));
    start_accept();

    PlayerList& list = PlayerList::get_instance();

    std::string wex = "wexelsdev";
    std::string pawmi = "Pawmi";
    std::string every = "offlinedata";

    std::string wexUuid = get_offline_UUID(wex);
    std::string pawmiUuid = get_offline_UUID(pawmi);
    std::string everyUuid = get_offline_UUID(every);

    std::string skin = "";

    list.add_player(wex, wexUuid, skin);
    list.add_player(pawmi, pawmiUuid, skin);
    list.add_player(every, everyUuid, skin);
}

Server& Server::get_instance() {
    return *instance;
}

toml::table& Server::get_config() {
    return config;
}

std::vector<uint8_t>& Server::get_public_key() const {
    return *public_key;
}

std::vector<uint8_t> Server::decrypt_rsa(std::vector<uint8_t>& data) const {
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

void Server::handle_accept(std::shared_ptr<Connection> new_connection, const boost::system::error_code& error) {
    if (!error) {
        new_connection->start();
    } else {
        LOG_ERROR("Accept error: " + error.message());
    }

    start_accept();
}