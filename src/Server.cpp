#include <iostream>
#include <memory>
#include <atomic>
#include <vector>
#include <string>
#include <random>
#include <future>
#include <mutex>
#include "Server.h"
#include "commands/CommandRegistry.h"
#include "console/Logger.h"
#include "game/player/PlayerList.h"
#include "crypto/RSA.h"
#include "../include/utils/ConfigValidator.h"
#include "game/world/WorldGenerator.h"
#include "network/PacketRegistry.h"
#include "commands/CommandRegistry.h"
#include "network/packet/outbound/play/DisconnectPacketPlay.h"

Server* Server::instance;

void register_all_packets(PacketRegistry& registry);
void register_all_commands(CommandRegistry& registry);

std::string get_random_splash() {
    std::vector<std::string> splashes = {
        "Revolutionizing the blocky world!",
        "Now with more blocks!",
        "C++ powered!",
        "As seen on TV!",
        "Awesome!",
        "100% pure!",
        "May contain nuts!",
        "Better than life!",
        "Heart-stoppingly good!"
    };
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, splashes.size() - 1);
    return splashes[distrib(gen)];
}

Server::Server(boost::asio::io_context& io_context)
    : acceptor_(io_context), io_context_(io_context), tick_timer_(io_context) {
    instance = this;

    std::cout << R"(               _   _               _
     /\       | | | |             (_)
    /  \   ___| |_| |__   ___ _ __ _ _   _ ___
   / /\ \ / _ \ __| '_ \ / _ \ '__| | | | / __|
  / ____ \  __/ |_| | | |  __/ |  | | |_| \__ \
 /_/    \_\___|\__|_| |_|\___|_|  |_|\__,_|___/
                                               )" << std::endl;
    std::cout << "Aetherius Core, Minecraft Version 1.16.5 - " << get_random_splash() << std::endl;
    std::cout << "This project is licensed under the GNU General Public License v3." << std::endl;
    std::cout << "Developed with ❤️ and C++ by wexels.dev, glitching.today, Pawmi." << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;

    LOG_INFO("Loading configuration...");
    config = ConfigValidator::load_and_validate("config.toml");
    LOG_INFO("Configuration loaded and validated.");

    const std::string worldName = config["world"]["name"].value_or("world");
    world = std::make_unique<World>(std::make_unique<FlatWorldGenerator>(), worldName);

    const int port = config["server"]["port"].value_or(25565);
    const std::string ip = config["server"]["ip"].value_or("0.0.0.0");
    LOG_INFO("Starting server on " + ip + ":" + std::to_string(port));

    AuthType authType = static_cast<AuthType>(config["server"]["auth_mode"].value_or(0));
    if (authType == AuthType::Offline) {
        LOG_WARN("SERVER IS RUNNING IN OFFLINE/INSECURE MODE!");
        LOG_WARN("The server will make no attempt to authenticate usernames. Beware.");
        LOG_WARN("While this makes the game possible to play without internet access, it also opens up the ability for hackers to connect with any username they choose.");
        LOG_WARN("To change this, set 'auth_mode' to '1' (Mojang) in the config.toml file.");
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

void Server::stop() {
    LOG_INFO("Stopping server...");

    acceptor_.close();

    for (const auto& player : PlayerList::getInstance().getPlayers()) {
        if (player) {
            player->disconnect("Server Closed");
        }
    }

    io_context_.stop();
    LOG_INFO("Server stopped.");
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
    LOG_INFO("Pre-generating spawn area...");
    const int radius = config["world"]["pregen_radius"].value_or(8);
    const int total_chunks = (2 * radius + 1) * (2 * radius + 1);
    std::atomic<int> chunks_generated = 0;
    int last_percentage = -1;
    std::mutex percentage_mutex;

    std::vector<std::future<void>> futures;

    for (int x = -radius; x <= radius; ++x) {
        for (int z = -radius; z <= radius; ++z) {
            auto promise = std::make_shared<std::promise<void>>();
            futures.push_back(promise->get_future());

            world->getOrGenerateChunk(x, z,
                [this, &chunks_generated, total_chunks, &last_percentage, &percentage_mutex, p = std::move(promise)](ChunkColumn* chunk) {
                if (chunk) {
                    int generated_count = ++chunks_generated;

                    std::lock_guard<std::mutex> lock(percentage_mutex);
                    int percentage = (generated_count * 100) / total_chunks;
                    if (percentage > last_percentage) {
                        last_percentage = percentage;
                        LOG_INFO("Pre-generating world: " + std::to_string(percentage) + "%");
                    }
                }
                p->set_value();
            });
        }
    }

    for(auto& f : futures) {
        f.get();
    }

    LOG_INFO("World pre-generation complete.");
}

void Server::start_accept() {
    std::shared_ptr<Connection> new_connection = Connection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
        [this, new_connection](const boost::system::error_code& error) {
            handle_accept(new_connection, error);
        });
}

void Server::init_rsa_async() {
    auto pkey = rsa::generate();
    std::vector<uint8_t> tempPublicKey = rsa::get_public_key(pkey);

    std::lock_guard<std::mutex> lock(rsa_mutex_);
    cur_rsa = pkey;
    public_key = std::make_unique<std::vector<uint8_t>>(tempPublicKey);
    rsa_initialized_ = true;
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
    tick_timer_.expires_after(std::chrono::milliseconds(50));
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
