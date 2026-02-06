#include "../../include/network/Http.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <regex>

using namespace boost::asio;
using tcp = ip::tcp;
namespace ssl = ssl;

HttpResponse http::get(const std::string& url)
{
    std::regex re(R"(^(https?)://([^/:]+)(:\d+)?(.*)$)");
    std::smatch m;

    if (!std::regex_match(url, m, re))
        throw std::runtime_error("Invalid URL");

    std::string scheme = m[1];
    std::string host   = m[2];
    std::string port   = m[3].matched ? m[3].str().substr(1)
                                      : (scheme == "https" ? "443" : "80");
    std::string path   = m[4].matched && !m[4].str().empty()
                           ? m[4].str()
                           : "/";

    io_context io;

    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(host, port);

    std::string request =
        "GET " + path + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Connection: close\r\n\r\n";

    HttpResponse result{};
    result.status = -1;

    auto process_response = [&](streambuf& buf)
    {
        std::istream is(&buf);
        std::string line;

        // Status line
        if (!std::getline(is, line))
            throw std::runtime_error("No HTTP response");

        if (line.back() == '\r')
            line.pop_back();

        std::stringstream ss(line);

        std::string proto;
        ss >> proto >> result.status;

        if (!ss || proto.rfind("HTTP/", 0) != 0)
            throw std::runtime_error("Invalid HTTP status line");

        // Headers
        std::string header;
        result.headers.clear();

        while (std::getline(is, header) && header != "\r")
            result.headers += header + "\n";

        // Body
        result.body.assign(
            std::istreambuf_iterator<char>(is),
            std::istreambuf_iterator<char>()
        );
    };

    if (scheme == "https")
    {
        ssl::context ctx(ssl::context::tls_client);
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        ssl::stream<tcp::socket> socket(io, ctx);

        connect(socket.next_layer(), endpoints);
        socket.handshake(ssl::stream_base::client);

        write(socket, buffer(request));

        streambuf buf;
        boost::system::error_code ec;

        while (read(socket, buf,
                    boost::asio::transfer_at_least(1), ec))
        {}

        if (ec != boost::asio::error::eof)
            throw boost::system::system_error(ec);

        process_response(buf);
    }
    else
    {
        tcp::socket socket(io);

        connect(socket, endpoints);

        write(socket, buffer(request));

        streambuf buf;
        boost::system::error_code ec;

        while (read(socket, buf,
                    boost::asio::transfer_at_least(1), ec))
        {}

        if (ec != boost::asio::error::eof)
            throw boost::system::system_error(ec);

        process_response(buf);
    }

    return result;
}