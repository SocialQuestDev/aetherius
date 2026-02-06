#pragma once
#include <openssl/evp.h>
#include <vector>

struct CryptoState {
    EVP_CIPHER_CTX* enc;
    EVP_CIPHER_CTX* dec;
};

namespace aes {
    CryptoState init_crypto(const std::vector<uint8_t>& secret);

    void encrypt(CryptoState& c,
                    uint8_t* data,
                    int len);

    void decrypt(CryptoState& c,
                    uint8_t* data,
                    int len);
}