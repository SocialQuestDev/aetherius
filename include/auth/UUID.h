#pragma once
#include <string>
#include <openssl/md5.h>
#include <iomanip>

struct UUID {
    uint64_t high;
    uint64_t low;

    bool operator==(const UUID & uuid) const {
        return high == uuid.high && low == uuid.low;
    }
};

std::string get_offline_UUID(const std::string& nickname);
UUID get_offline_UUID_128(const std::string& nickname);
UUID string_to_uuid(const std::string& uuid);
std::string uuid_to_string(uint64_t high, uint64_t low);