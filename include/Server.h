#pragma once

#include <boost/asio.hpp>
#include <toml++/toml.hpp>
#include <memory>
#include <mutex>
#include "network/Connection.h"
#include "network/PacketRegistry.h"
#include "game/world/World.h"
#include "crypto/RSA.h"
#include "commands/CommandRegistry.h"
#include "console/ConsoleManager.h"

using boost::asio::ip::tcp;

class Server {
public:
    Server(boost::asio::io_context& io_context);
    static Server& get_instance();
    toml::table& get_config();
    toml::table& get_world_config();
    std::vector<uint8_t>& get_public_key() const;
    std::vector<uint8_t> decrypt_rsa(const std::vector<uint8_t>& data) const;
    World& get_world();
    PacketRegistry& get_packet_registry();
    CommandRegistry& get_command_registry();
    void start_console();
    void async_init();
    void start_tick_system();

private:
    void start_accept();
    void handle_accept(const std::shared_ptr<Connection>& new_connection, const boost::system::error_code& error);
    void register_packets();
    void register_commands();
    void init_rsa_async();
    void tick();

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    std::unique_ptr<ConsoleManager> console_manager_;
    toml::table config;
    static Server* instance;
    RSA* cur_rsa;
    std::unique_ptr<std::vector<uint8_t>> public_key;
    std::unique_ptr<World> world;
    PacketRegistry packet_registry_;
    CommandRegistry command_registry_;
    bool rsa_initialized_ = false;
    std::mutex rsa_mutex_;

    // Tick system (20 ticks per second = 50ms per tick)
    boost::asio::steady_timer tick_timer_;
};
