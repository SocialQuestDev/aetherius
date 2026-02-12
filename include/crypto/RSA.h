#pragma once
#include <openssl/evp.h>
#include <vector>
#include <string>

namespace rsa {
    EVP_PKEY* generate();
    std::vector<uint8_t> get_public_key(EVP_PKEY* pkey);
    std::vector<uint8_t> decrypt(EVP_PKEY* pkey, const std::vector<uint8_t>& data);
    std::string get_private_key_pem(EVP_PKEY* pkey);
}
