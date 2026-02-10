#pragma once

#include "PacketBuffer.h"
#include "packet/OutboundPacket.h"
#include "../crypto/AES.h"
#include "../game/player/Player.h"

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

    void set_waiting_for_encryption(bool waiting);
    bool is_waiting_for_encryption() const;

    bool is_connected() const;
    void enable_encryption(const std::vector<uint8_t>& shared_secret);

    void set_compression(bool enabled);
    void setPlayer(std::shared_ptr<Player> player);
    std::shared_ptr<Player> getPlayer() const;

    void send_join_game();
    void start_keep_alive_timer();

private:
    explicit Connection(boost::asio::io_context& io_context);

    void do_read();
    void handle_packet(std::vector<uint8_t>& rawData);
    void send_light_data(int chunkX, int chunkZ);
    void send_keep_alive();

    std::unique_ptr<CryptoState> crypto_state;
    std::unique_ptr<std::vector<uint8_t>> verify_token;

    bool waiting_for_response = false;
    bool connected = false;
    bool encrypt = false;
    bool just_enabled_encryption = false;
    bool compression_enabled = false;
    bool waitingForResponse = false;
    std::string nickname;
    std::vector<uint8_t> stream_buffer_;
    tcp::socket socket_;
    uint8_t buffer_[4096]{};
    State state_ = State::HANDSHAKE;
    uint64_t last_keep_alive_id_;
    boost::asio::steady_timer keep_alive_timer_;

    std::shared_ptr<Player> player;
    // Добавь это в секцию private:
    std::vector<uint8_t> incoming_buffer_; // Буфер для склейки кусков пакетов
    void process_incoming_buffer();        // Метод для нарезки потока на пакеты
};