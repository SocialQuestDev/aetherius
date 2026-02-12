#include "crypto/AES.h"
#include <stdexcept>

CryptoState aes::init_crypto(const std::vector<uint8_t>& secret)
{
    CryptoState c;

    c.enc.reset(EVP_CIPHER_CTX_new());
    c.dec.reset(EVP_CIPHER_CTX_new());

    if (!c.enc || !c.dec) {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
    }

    if (1 != EVP_EncryptInit_ex(c.enc.get(), EVP_aes_128_cfb8(), nullptr, secret.data(), secret.data())) {
        throw std::runtime_error("Failed to initialize AES encryption");
    }

    if (1 != EVP_DecryptInit_ex(c.dec.get(), EVP_aes_128_cfb8(), nullptr, secret.data(), secret.data())) {
        throw std::runtime_error("Failed to initialize AES decryption");
    }

    return c;
}

void aes::encrypt(CryptoState& c, uint8_t* data, int len)
{
    int out_len;
    if (1 != EVP_EncryptUpdate(c.enc.get(), data, &out_len, data, len)) {
        throw std::runtime_error("AES encryption failed");
    }
}

void aes::decrypt(CryptoState& c, uint8_t* data, int len)
{
    int out_len;
    if (1 != EVP_DecryptUpdate(c.dec.get(), data, &out_len, data, len)) {
        throw std::runtime_error("AES decryption failed");
    }
}
