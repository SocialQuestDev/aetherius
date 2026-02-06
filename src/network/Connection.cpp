#include "../../include/network/Connection.h"
#include "../../include/Logger.h"
#include "../../include/game/world/Chunk.h"
#include "../../include/game/world/World.h"
#include "../../include/Server.h"
#include "../../include/crypto/RSA.h"
#include "../../include/auth/MojangAuthHelper.h"
#include "../../include/utility/VectorUtilities.h"
#include "../../include/game/player/PlayerList.h"
#include "../../include/game/player/Player.h"

#include <toml++/toml.hpp>
#include <nlohmann/json.hpp>
#include <openssl/md5.h>
#include <zlib.h>

using json = nlohmann::json;

Connection::Connection(boost::asio::io_context& io_context)
    : socket_(io_context) {}

std::shared_ptr<Connection> Connection::create(boost::asio::io_context& io_context) {
    return std::shared_ptr<Connection>(new Connection(io_context));
}

tcp::socket& Connection::socket() {
    return socket_;
}

void Connection::start() {
    do_read();
}

static std::string base64_encode_local(const std::vector<unsigned char>& data) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

static std::string getIconBase64(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
    return "data:image/png;base64," + base64_encode_local(buffer);
}

void Connection::do_read() {
    auto self(shared_from_this());

    socket_.async_read_some(boost::asio::buffer(buffer_),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::vector<uint8_t> received(buffer_, buffer_ + length);

                if (encrypt && crypto_state) {
                    aes::decrypt(*crypto_state, received.data(), (int)received.size());
                }

                try {
                    handle_packet(received);
                } catch (std::exception& e) {
                    LOG_ERROR("Packet parsing error: " + std::string(e.what()));
                }

                do_read();
            } else {
                LOG_INFO("Connection closed or error: " + ec.message());
            }
        });
}

void Connection::handle_packet(std::vector<uint8_t>& rawData) {
    PacketBuffer raw(rawData);

    try {
        Server& server = Server::get_instance();

        if (!config) {
            toml::table cfg = server.get_config();
            config = std::make_unique<toml::table>(cfg);
        }

        toml::table& serverCfg = *config;

        if (raw.data.size() == 1) {
            if (raw.data[0] == 0xFE) {
                LOG_DEBUG("Received Legacy Ping (0xFE), ignoring.");
                return;
            }
            LOG_DEBUG("Received 1-byte packet: " + std::format("0x{:02X}", raw.data[0]));
            return;
        }

        while (raw.readerIndex < raw.data.size()) {
            int packetLength = raw.readVarInt();
            if (packetLength < 1) break;

            std::vector<uint8_t> payload;

            if (compression_enabled) {
                size_t startIdx = raw.readerIndex;
                int dataLength = raw.readVarInt();
                int dataLenFieldSize = (int)(raw.readerIndex - startIdx);

                int remainingBytes = packetLength - dataLenFieldSize;

                if (dataLength > 0) {
                    payload.resize(dataLength);
                    unsigned long destLen = dataLength;

                    int res = uncompress(payload.data(), &destLen,
                                       raw.data.data() + raw.readerIndex,
                                       remainingBytes);

                    if (res != Z_OK) {
                        LOG_ERROR("Zlib decompression failed: " + std::to_string(res));
                        break;
                    }
                    raw.readerIndex += remainingBytes;
                } else {
                    payload.assign(raw.data.begin() + raw.readerIndex,
                                   raw.data.begin() + raw.readerIndex + remainingBytes);
                    raw.readerIndex += remainingBytes;
                }
            } else {
                payload.assign(raw.data.begin() + raw.readerIndex,
                               raw.data.begin() + raw.readerIndex + packetLength);
                raw.readerIndex += packetLength;
            }

            PacketBuffer reader(payload);
            if (payload.empty()) continue;
            int packetID = reader.readVarInt();

            if (state_ == State::HANDSHAKE) {
                if (packetID == 0x00) { // Handshake Packet
                    int protocolVersion = reader.readVarInt();
                    std::string serverAddr = reader.readString();
                    unsigned short serverPort = reader.readUShort();
                    int nextState = reader.readVarInt();

                    LOG_DEBUG("Handshake: Protocol " + std::to_string(protocolVersion) +
                                 " NextState: " + std::to_string(nextState));

                    if (nextState == 1) state_ = State::STATUS;
                    else if (nextState == 2) state_ = State::LOGIN;
                }
            }
            else if (state_ == State::STATUS) {
                if (packetID == 0x00) { // Status Request
                    std::string motd = serverCfg["server"]["motd"].value_or("Aetherius Server");
                    std::string icon = getIconBase64(serverCfg["server"]["icon_path"].value_or("server-icon.png"));

                    json response;
                    PlayerList& list = PlayerList::get_instance();

                    response["version"]["name"] = "1.16.5";
                    response["version"]["protocol"] = 754;
                    response["players"]["max"] = list.get_max_players();
                    response["players"]["online"] = list.get_players_count();
                    json playerSample = json::array();

                    if (list.get_max_players() > 0) {
                        for (int i = 0; i < list.get_players_count(); ++i) {
                            Player* p = list.get_player(i);
                            playerSample.push_back({
                                {"name", p->get_nickname()},
                                {"id", p->get_uuid()}
                            });
                        }
                    }

                    response["players"]["sample"] = playerSample;
                    response["description"]["text"] = motd;

                    if (!icon.empty()) {
                        response["favicon"] = icon;
                    }

                    std::string jsonResponse = response.dump();

                    PacketBuffer res;
                    res.writeVarInt(0x00);
                    res.writeString(jsonResponse);
                    send_packet(res);
                }
                else if (packetID == 0x01) { // Status Ping
                    int64_t pingPayload = reader.readLong();
                    LOG_DEBUG("Received ping with nonce: " + std::to_string(pingPayload) + ", sending response");
                    PacketBuffer pong;
                    pong.writeVarInt(0x01);
                    pong.writeLong(pingPayload);
                    send_packet(pong);
                }
            }
            else if (state_ == State::LOGIN) {
                if (packetID == 0x00) { // Login Start
                    std::string playerName = reader.readString();
                    nickname = playerName;
                    LOG_INFO("Player logging in: " + playerName);

                    if (connected) {
                        PacketBuffer alreadyJoinedDisconnect;
                        alreadyJoinedDisconnect.writeVarInt(0x19);
                        alreadyJoinedDisconnect.writeString("{\"text\":\"Aetherius: Ты уже на сервере!\"}");
                        send_packet(alreadyJoinedDisconnect);
                        break;
                    }

                    if (serverCfg["server"]["online_mode"].value_or(false)) {
                        std::vector<uint8_t> verifyTokenTemp = auth::generate_verify_token();
                        verify_token = std::make_unique<std::vector<uint8_t>>(verifyTokenTemp);

                        PacketBuffer encReq;
                        encReq.writeVarInt(0x01);
                        encReq.writeString(""); // server ID
                        encReq.writeByteArray(server.get_public_key());
                        encReq.writeByteArray(*verify_token);

                        waitingForResponse = true;
                        send_packet(encReq);
                        break;
                    }
                    else {
                        if (serverCfg["server"]["compression_enabled"].value_or(false)) {
                            int compression_threshold = serverCfg["server"]["compression_threshold"].value_or(256);

                            PacketBuffer compPacket;
                            compPacket.writeVarInt(0x03); // Set Compression
                            compPacket.writeVarInt(compression_threshold);

                            std::vector<uint8_t> raw = compPacket.finalize(false, -1, nullptr);
                            send_packet_raw(raw);

                            compression_enabled = true;
                        }

                        UUID uuid = get_offline_UUID_128(playerName);
                        std::string uuidStr = uuid_to_string(uuid.high, uuid.low);

                        PacketBuffer loginSuccess;
                        loginSuccess.writeVarInt(0x02);
                        loginSuccess.writeUUID(uuid);
                        loginSuccess.writeString(playerName);
                        send_packet(loginSuccess);

                        std::string empty = "";

                        PlayerList::get_instance().add_player(nickname, uuidStr, empty);
                        connected = true;
                        state_ = State::PLAY;
                    }
                }
                else if (packetID == 0x01) { // Encryption Response
                    if (!waitingForResponse || !verify_token) {
                        LOG_ERROR("Unexpected encryption response");
                        break;
                    }

                    std::vector<uint8_t> encryptedSecret = reader.readByteArray();
                    std::vector<uint8_t> encryptedToken = reader.readByteArray();

                    std::vector<uint8_t> secret = server.decrypt_rsa(encryptedSecret);
                    std::vector<uint8_t> token = server.decrypt_rsa(encryptedToken);

                    if (!vectors_equal(token, *verify_token)) {
                        LOG_ERROR("Verify token mismatch");
                        break;
                    }

                    crypto_state = std::make_unique<CryptoState>(aes::init_crypto(secret));

                    if (serverCfg["server"]["compression_enabled"].value_or(false)) {
                        int compression_threshold = serverCfg["server"]["compression_threshold"].value_or(256);
                        PacketBuffer compPacket;
                        compPacket.writeVarInt(0x03);
                        compPacket.writeVarInt(compression_threshold);

                        send_packet_raw(compPacket.finalize(false, -1, crypto_state.get()));

                        compression_enabled = true;
                    }

                    PlayerData data = auth::get_uuid(nickname);

                    PacketBuffer loginSuccess;
                    loginSuccess.writeVarInt(0x02);
                    loginSuccess.writeUUID(data.uuid);
                    loginSuccess.writeString(nickname);

                    send_packet(loginSuccess);

                    encrypt = true;
                    state_ = State::PLAY;

                    LOG_DEBUG("Online auth completed, encryption enabled.");
                    return;
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Packet error: " + std::string(e.what()));
    }
}

void Connection::send_packet(PacketBuffer& packet) {
    toml::table cfg = *config;

    std::vector<uint8_t> finalData = packet.finalize(compression_enabled, cfg["server"]["compression_threshold"].value_or(256), crypto_state.get());

    send_packet_raw(finalData);
}

void Connection::send_packet_raw(std::vector<uint8_t> packetData) {
    auto self(shared_from_this());

    auto sharedData = std::make_shared<std::vector<uint8_t>>(std::move(packetData));

    boost::asio::async_write(socket_, boost::asio::buffer(*sharedData),
        [this, self, sharedData](boost::system::error_code ec, std::size_t length) {
            if (ec) {
                LOG_ERROR("Send failed: " + ec.message());
            }
        });
}