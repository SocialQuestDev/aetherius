#include <iostream>
#include <boost/asio.hpp>
#include "../include/Server.h"
#include "../include/console/Logger.h"

int main() {
    try {
        boost::asio::io_context io_context;

        Server server(io_context);
        server.start_console();

        io_context.run();
    } catch (std::exception& e) {
        LOG_FATAL(std::string("Exception: ") + e.what());
    }

    return 0;
}