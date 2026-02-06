#include "../../include/crypto/AES.h"

CryptoState aes::init_crypto(const std::vector<uint8_t>& secret)
{
    CryptoState c;

    c.enc = EVP_CIPHER_CTX_new();
    c.dec = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(
        c.enc,
        EVP_aes_128_cfb8(),
        nullptr,
        secret.data(),
        secret.data()
    );

    EVP_DecryptInit_ex(
        c.dec,
        EVP_aes_128_cfb8(),
        nullptr,
        secret.data(),
        secret.data()
    );

    return c;
}

void aes::encrypt(CryptoState& c,
                    uint8_t* data,
                    int len)
{
    int out_len;

    EVP_EncryptUpdate(
        c.enc,
        data,
        &out_len,
        data,
        len
    );
}

void aes::decrypt(CryptoState& c,
                    uint8_t* data,
                    int len)
{
    int out_len;

    EVP_DecryptUpdate(
        c.dec,
        data,
        &out_len,
        data,
        len
    );
}