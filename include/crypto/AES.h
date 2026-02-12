#pragma once
#include <openssl/evp.h>
#include <vector>
#include <memory>

struct EVP_CIPHER_CTX_deleter {
    void operator()(EVP_CIPHER_CTX* p) const { EVP_CIPHER_CTX_free(p); }
};

using unique_ctx_t = std::unique_ptr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_deleter>;

struct CryptoState {
    unique_ctx_t enc;
    unique_ctx_t dec;
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
