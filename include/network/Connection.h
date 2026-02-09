#pragma once
#include "PacketBuffer.h"
#include "../crypto/AES.h"

#include <memory>
#include <boost/asio.hpp>
#include <vector>
#include <toml++/toml.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>

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
    void handle_packet(std::vector<uint8_t>& rawData);
    void process_packets();
    void send_packet(PacketBuffer& packet);
    void send_packet_raw(std::vector<uint8_t> packetData);
    void send_join_game();
    void send_light_data(int chunkX, int chunkZ);
    void send_keep_alive();
    void start_keep_alive_timer();
    void kill_player();
    void teleport_to_spawn();

    std::unique_ptr<CryptoState> crypto_state;
    std::unique_ptr<std::vector<uint8_t>> verify_token;

    bool connected = false;
    bool encrypt = false;
    bool just_enabled_encryption = false;
    bool compression_enabled = false;
    bool waitingForResponse = false;
    std::string nickname = "";
    std::vector<uint8_t> stream_buffer_;
    tcp::socket socket_;
    uint8_t buffer_[4096]{};
    State state_ = State::HANDSHAKE;
    uint64_t last_keep_alive_id_;
    boost::asio::steady_timer keep_alive_timer_;
    bool is_dead = false;
};