#include <memory>
#include "../include/Server.h"
#include "../include/Logger.h"

Server::Server(boost::asio::io_context& io_context, short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    
    LOG_INFO("Server starting on port " + std::to_string(port));
    start_accept();
}

void Server::start_accept() {
    std::shared_ptr<Connection> new_connection = Connection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
        [this, new_connection](const boost::system::error_code& error) {
            handle_accept(new_connection, error);
        });
}

void Server::handle_accept(std::shared_ptr<Connection> new_connection, const boost::system::error_code& error) {
    if (!error) {
        new_connection->start();
    } else {
        LOG_ERROR("Accept error: " + error.message());
    }

    start_accept();
}