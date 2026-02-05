#pragma once
#include <memory>
#include <boost/asio.hpp>
#include <vector>

using boost::asio::ip::tcp;

enum class State {
    HANDSHAKE,
    STATUS,
    LOGIN,
    PLAY
};

class Connection : public std::enable_shared_from_this<Connection> {
public:
    static std::shared_ptr<Connection> create(boost::asio::io_context& io_context);
    tcp::socket& socket();
    void start();

private:
    explicit Connection(boost::asio::io_context& io_context);

    void do_read();
    void handle_packet(std::vector<uint8_t>& buffer);
    void send_packet(std::vector<uint8_t> packetData);

    tcp::socket socket_;
    uint8_t buffer_[4096];
    State state_ = State::HANDSHAKE;
};