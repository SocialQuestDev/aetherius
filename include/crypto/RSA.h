#pragma once
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <vector>
#include <string>

namespace rsa {
    RSA* generate();
    std::vector<uint8_t> get_public_key(RSA* rsa);
    std::vector<uint8_t> decrypt(RSA* rsa, const std::vector<uint8_t>& data);
    std::string get_private_key_pem(RSA* rsa);
}