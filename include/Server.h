#pragma once

#include <boost/asio.hpp>
#include <toml++/toml.hpp>
#include <memory>
#include "network/Connection.h"
#include "game/world/World.h"
#include "crypto/RSA.h"

using boost::asio::ip::tcp;

class Server {
public:
    Server(boost::asio::io_context& io_context);
    static Server& get_instance();
    toml::table& get_config();
    std::vector<uint8_t>& get_public_key() const;
    std::vector<uint8_t> decrypt_rsa(const std::vector<uint8_t>& data) const;
    World& get_world();

private:
    void start_accept();
    void handle_accept(const std::shared_ptr<Connection>& new_connection, const boost::system::error_code& error);

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    toml::table config;
    static Server* instance;
    RSA* cur_rsa;
    std::unique_ptr<std::vector<uint8_t>> public_key;
    std::unique_ptr<World> world;
};
