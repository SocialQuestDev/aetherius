#include "../../include/auth/UUID.h"
#include <ios>

std::string get_offline_UUID(const std::string& nickname) {
    if (nickname.size() > 1024)
        throw std::runtime_error("Nickname too long");

    std::string input = "OfflinePlayer:" + nickname;
    unsigned char hash[MD5_DIGEST_LENGTH];

    MD5((unsigned char*)input.c_str(), input.length(), hash);

    hash[6] = (hash[6] & 0x0f) | 0x30;
    hash[8] = (hash[8] & 0x3f) | 0x80;

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        ss << std::setw(2) << (int)hash[i];
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            ss << "-";
        }
    }

    return ss.str();
}

UUID get_offline_UUID_128(const std::string& nickname) {
    if (nickname.size() > 1024)
        throw std::runtime_error("Nickname too long");

    std::string input = "OfflinePlayer:" + nickname;
    unsigned char hash[MD5_DIGEST_LENGTH];

    MD5(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);

    hash[6] = (hash[6] & 0x0F) | 0x30;
    hash[8] = (hash[8] & 0x3F) | 0x80;

    uint64_t high = 0;
    uint64_t low = 0;

    for (int i = 0; i < 8; ++i) {
        high = (high << 8) | hash[i];
    }
    for (int i = 8; i < 16; ++i) {
        low = (low << 8) | hash[i];
    }

    return {high, low};
}

UUID string_to_uuid(const std::string& uuid) {
    std::string clean;
    clean.reserve(32);

    for (char c : uuid) {
        if (c != '-') clean.push_back(c);
    }

    if (clean.size() != 32)
        throw std::runtime_error("Invalid UUID length");

    uint64_t high = 0, low = 0;
    for (int i = 0; i < 16; ++i) {
        uint8_t byte = static_cast<uint8_t>(std::stoi(clean.substr(i*2, 2), nullptr, 16));
        if (i < 8) high = (high << 8) | byte;
        else       low  = (low  << 8) | byte;
    }

    return {high, low};
}

std::string uuid_to_string(const uint64_t high, const uint64_t low) {
    unsigned char bytes[16];

    for (int i = 0; i < 8; ++i) {
        bytes[7 - i] = static_cast<unsigned char>((high >> (i * 8)) & 0xFF);
    }
    for (int i = 0; i < 8; ++i) {
        bytes[15 - i] = static_cast<unsigned char>((low >> (i * 8)) & 0xFF);
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            ss << "-";
        }
    }

    return ss.str();
}