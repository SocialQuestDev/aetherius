#include <iostream>
#include <boost/asio.hpp>
#include "Server.h"
#include "console/Logger.h"

int main() {
    try {
        boost::asio::io_context io_context;

        Server server(io_context);
        server.start_console();
        server.start_tick_system();

        io_context.run();
    } catch (std::exception& e) {
        LOG_FATAL(std::string("Exception: ") + e.what());
    }

    return 0;
}