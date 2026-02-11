#include "crypto/RSA.h"
#include <openssl/err.h>
#include <stdexcept>

RSA* rsa::generate() {
    RSA* rsa = RSA_new();
    BIGNUM* e = BN_new();

    BN_set_word(e, RSA_F4); // 65537
    RSA_generate_key_ex(rsa, 1024, e, nullptr);

    BN_free(e);
    return rsa;
}

std::vector<uint8_t> rsa::get_public_key(RSA* rsa) {
    int len = i2d_RSA_PUBKEY(rsa, nullptr);

    std::vector<uint8_t> buf(len);
    unsigned char* p = buf.data();

    i2d_RSA_PUBKEY(rsa, &p);

    return buf;
}

std::vector<uint8_t> rsa::decrypt(RSA* rsa, const std::vector<uint8_t>& data) {
    if (!rsa) throw std::runtime_error("RSA key not initialized");

    std::vector<uint8_t> out(RSA_size(rsa));
    int len = RSA_private_decrypt(
        static_cast<int>(data.size()),
        data.data(),
        out.data(),
        rsa,
        RSA_PKCS1_PADDING
    );

    if (len == -1) {
        unsigned long err = ERR_get_error();
        throw std::runtime_error("RSA decryption failed: " + std::string(ERR_error_string(err, nullptr)));
    }

    out.resize(len);
    return out;
}

std::string rsa::get_private_key_pem(RSA* rsa) {
    if (!rsa) throw std::runtime_error("RSA key not initialized");

    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPrivateKey(bio, rsa, nullptr, nullptr, 0, nullptr, nullptr);

    char* dataPtr = nullptr;
    long len = BIO_get_mem_data(bio, &dataPtr);
    std::string pem(dataPtr, len);
    BIO_free(bio);

    return pem;
}