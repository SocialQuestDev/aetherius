#pragma once

#include "network/PacketBuffer.h"
#include "packet/OutboundPacket.h"
#include "crypto/AES.h"
#include "game/player/Player.h"

#include <memory>
#include <boost/asio.hpp>
#include <vector>
#include <queue>
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
    void setState(State state);
    State getState() const;
    void send_packet(PacketBuffer& packet);
    void send_packet(OutboundPacket& packet);
    void send_packet_raw(std::vector<uint8_t> packetData);
    std::vector<uint8_t> finalize_packet(PacketBuffer& packet);

    void set_verify_token(const std::vector<uint8_t>& token);
    std::vector<uint8_t> get_verify_token() const;

    void set_nickname(const std::string& name);
    std::string get_nickname() const;

    void set_protocol_version(int version);
    int get_protocol_version() const;

    void set_waiting_for_encryption_response(bool waiting);
    bool is_waiting_for_encryption_response() const;

    bool is_connected() const;
    void enable_encryption(const std::vector<uint8_t>& shared_secret);

    void set_compression(bool enabled);
    void setPlayer(std::shared_ptr<Player> player);
    std::shared_ptr<Player> getPlayer() const;

    void send_join_game();
    void broadcast_player_join();
    void start_keep_alive_timer();
    void handle_keep_alive(uint64_t id);
    int getPing() const;
    void update_chunks();

    boost::asio::io_context::strand& get_write_strand();

    std::chrono::steady_clock::time_point last_keep_alive_sent_;
    int ping_ms_ = 0;
    int last_chunk_x_ = 0;
    int last_chunk_z_ = 0;

private:
    explicit Connection(boost::asio::io_context& io_context);

    void do_read();
    void send_light_data(int chunkX, int chunkZ);
    void send_keep_alive();

    std::unique_ptr<CryptoState> crypto_state;
    std::unique_ptr<std::vector<uint8_t>> verify_token;

    bool connected = false;
    bool encrypt = false;
    bool compression_enabled = false;
    bool waiting_for_keep_alive_ = false;
    bool waiting_for_encryption_response_ = false;
    int protocol_version = 751;
    std::string nickname;
    std::vector<uint8_t> stream_buffer_;
    tcp::socket socket_;
    uint8_t buffer_[4096]{};
    State state_ = State::HANDSHAKE;
    uint64_t last_keep_alive_id_ = 0;
    boost::asio::steady_timer keep_alive_timer_;
    boost::asio::io_context::strand write_strand_;

    std::shared_ptr<Player> player;
    std::vector<uint8_t> incoming_buffer_;
    std::queue<std::shared_ptr<std::vector<uint8_t>>> write_queue_;
    bool writing_ = false;
    void process_incoming_buffer();
    void do_write();
};
