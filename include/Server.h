#pragma once

#include <boost/asio.hpp>
#include <toml++/toml.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
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
    void stop();
    void start_systems();
    void wait_for_systems();
    toml::table& get_config();
    toml::table& get_world_config();
    std::vector<uint8_t>& get_public_key() const;
    std::vector<uint8_t> decrypt_rsa(const std::vector<uint8_t>& data) const;
    World& get_world();
    PacketRegistry& get_packet_registry();
    CommandRegistry& get_command_registry();
    boost::asio::io_context& get_io_context();
    boost::asio::io_context& get_game_io_context();
    void post_game_task(std::function<void()> task);
    void start_console();
    void async_init();
    void start_tick_system();

private:
    void start_accept();
    void handle_accept(const std::shared_ptr<Connection>& new_connection, const boost::system::error_code& error);
    void register_packets();
    void register_commands();
    void init_rsa_async();
    void pre_generate_world();
    void tick();

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    boost::asio::io_context game_io_context_;
    boost::asio::io_context console_io_context_;
    std::unique_ptr<ConsoleManager> console_manager_;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> game_work_;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> console_work_;
    std::thread game_thread_;
    std::thread console_thread_;
    std::atomic<bool> systems_started_{false};
    toml::table config;
    static Server* instance;
    EVP_PKEY* cur_rsa;
    std::unique_ptr<std::vector<uint8_t>> public_key;
    std::unique_ptr<World> world;
    PacketRegistry packet_registry_;
    CommandRegistry command_registry_;
    bool rsa_initialized_ = false;
    std::mutex rsa_mutex_;

    boost::asio::steady_timer tick_timer_;
};
