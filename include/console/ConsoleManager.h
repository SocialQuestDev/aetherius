#pragma once

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/streambuf.hpp>

class ConsoleManager {
public:
    ConsoleManager(boost::asio::io_context& io_context);
    void start();

private:
    void do_read();
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);

    boost::asio::posix::stream_descriptor input_;
    boost::asio::streambuf buffer_;
};