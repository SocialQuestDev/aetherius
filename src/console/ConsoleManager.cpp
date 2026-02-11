#include "console/ConsoleManager.h"
#include "Server.h"
#include <iostream>

ConsoleManager::ConsoleManager(boost::asio::io_context& io_context)
    : input_(io_context, ::dup(STDIN_FILENO)) {}

void ConsoleManager::start() {
    do_read();
}

void ConsoleManager::do_read() {
    boost::asio::async_read_until(input_, buffer_, '\n',
        [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
            handle_read(error, bytes_transferred);
        });
}

void ConsoleManager::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error) {
        std::istream is(&buffer_);
        std::string line;
        std::getline(is, line);

        if (!line.empty()) {
            if (line.back() == '\r') {
                line.pop_back();
            }
            Server::get_instance().get_command_registry().executeCommand(nullptr, line);
        }

        do_read();
    }
}