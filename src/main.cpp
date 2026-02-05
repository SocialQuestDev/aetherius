#include <iostream>
#include <boost/asio.hpp>
#include "../include/Server.h"
#include "../include/Logger.h"

int main() {
    try {
        boost::asio::io_context io_context;

        Server server(io_context, 25565);

        io_context.run();
    } catch (std::exception& e) {
        Logger::error(std::string("Exception: ") + e.what());
    }

    return 0;
}