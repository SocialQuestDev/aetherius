#include <iostream>
#include <boost/asio.hpp>
#include <csignal>
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

        server.start_console();
        server.start_tick_system();

        io_context.run();
    } catch (std::exception& e) {
        LOG_FATAL(std::string("Exception: ") + e.what());
    }

    return 0;
}
