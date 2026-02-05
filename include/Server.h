#pragma once
#include <boost/asio.hpp>
#include <fstream>
#include <vector>
#include <string>
#include "Connection.h"

using boost::asio::ip::tcp;

class Server {
public:
    Server(boost::asio::io_context& io_context, short port);

    std::string base64_encode(const std::vector<unsigned char>& data) {
        static const char sEncodingTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        int val = 0, valb = -6;
        for (unsigned char c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(sEncodingTable[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(sEncodingTable[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    std::string getIconBase64(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return "";
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
        return "data:image/png;base64," + base64_encode(buffer);
    }

private:
    void start_accept();
    void handle_accept(std::shared_ptr<Connection> new_connection, const boost::system::error_code& error);

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
};