#include <iostream>
#include <boost/asio.hpp>
#include <csignal>
#include <thread>
#include <vector>
#include <algorithm>
#include "Server.h"
#include "console/Logger.h"

Server* server_instance = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOG_INFO("Shutdown signal received. Stopping server...");
        if (server_instance) {
            server_instance->stop();
        }
    }
}

int main() {
    try {
        boost::asio::io_context io_context;

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        Server server(io_context);
        server_instance = &server;

        server.start_systems();

        const unsigned int hw_threads = std::max(2u, std::thread::hardware_concurrency());
        std::vector<std::thread> net_threads;
        net_threads.reserve(hw_threads);
        for (unsigned int i = 0; i < hw_threads; ++i) {
            net_threads.emplace_back([&io_context]() { io_context.run(); });
        }
        for (auto& t : net_threads) {
            t.join();
        }
        server.wait_for_systems();
    } catch (std::exception& e) {
        LOG_FATAL(std::string("Exception: ") + e.what());
    }

    return 0;
}
