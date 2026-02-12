#include "crypto/RSA.h"
#include <openssl/err.h>
#include <openssl/pem.h>
#include <stdexcept>
#include <memory>

struct EVP_PKEY_deleter {
    void operator()(EVP_PKEY* p) const { EVP_PKEY_free(p); }
};
struct EVP_PKEY_CTX_deleter {
    void operator()(EVP_PKEY_CTX* p) const { EVP_PKEY_CTX_free(p); }
};
struct BIO_deleter {
    void operator()(BIO* p) const { BIO_free_all(p); }
};

using unique_pkey_t = std::unique_ptr<EVP_PKEY, EVP_PKEY_deleter>;
using unique_pkey_ctx_t = std::unique_ptr<EVP_PKEY_CTX, EVP_PKEY_CTX_deleter>;
using unique_bio_t = std::unique_ptr<BIO, BIO_deleter>;

EVP_PKEY* rsa::generate() {
    unique_pkey_ctx_t ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_PKEY_CTX");
    }
    if (EVP_PKEY_keygen_init(ctx.get()) <= 0) {
        throw std::runtime_error("Failed to initialize keygen");
    }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 1024) <= 0) {
        throw std::runtime_error("Failed to set RSA keygen bits");
    }

    EVP_PKEY* pkey_raw = nullptr;
    if (EVP_PKEY_keygen(ctx.get(), &pkey_raw) <= 0) {
        throw std::runtime_error("Failed to generate key");
    }
    return pkey_raw;
}

std::vector<uint8_t> rsa::get_public_key(EVP_PKEY* pkey) {
    int len = i2d_PUBKEY(pkey, nullptr);
    if (len <= 0) {
        throw std::runtime_error("Failed to determine public key length");
    }
    std::vector<uint8_t> buf(len);
    unsigned char* p = buf.data();
    if (i2d_PUBKEY(pkey, &p) <= 0) {
        throw std::runtime_error("Failed to serialize public key");
    }
    return buf;
}

std::vector<uint8_t> rsa::decrypt(EVP_PKEY* pkey, const std::vector<uint8_t>& data) {
    if (!pkey) throw std::runtime_error("EVP_PKEY not initialized");

    unique_pkey_ctx_t ctx(EVP_PKEY_CTX_new(pkey, nullptr));
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_PKEY_CTX for decryption");
    }
    if (EVP_PKEY_decrypt_init(ctx.get()) <= 0) {
        throw std::runtime_error("Failed to initialize decryption");
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) <= 0) {
        throw std::runtime_error("Failed to set RSA padding");
    }

    size_t out_len;
    if (EVP_PKEY_decrypt(ctx.get(), nullptr, &out_len, data.data(), data.size()) <= 0) {
        throw std::runtime_error("Failed to determine output buffer size for decryption");
    }

    std::vector<uint8_t> out(out_len);
    if (EVP_PKEY_decrypt(ctx.get(), out.data(), &out_len, data.data(), data.size()) <= 0) {
        unsigned long err = ERR_get_error();
        throw std::runtime_error("RSA decryption failed: " + std::string(ERR_error_string(err, nullptr)));
    }

    out.resize(out_len);
    return out;
}

std::string rsa::get_private_key_pem(EVP_PKEY* pkey) {
    if (!pkey) throw std::runtime_error("EVP_PKEY not initialized");

    unique_bio_t bio(BIO_new(BIO_s_mem()));
    if (!bio) {
        throw std::runtime_error("Failed to create BIO");
    }
    if (PEM_write_bio_PrivateKey(bio.get(), pkey, nullptr, nullptr, 0, nullptr, nullptr) <= 0) {
        throw std::runtime_error("Failed to write private key to BIO");
    }

    char* dataPtr = nullptr;
    long len = BIO_get_mem_data(bio.get(), &dataPtr);
    if (len <= 0) {
        return "";
    }
    return std::string(dataPtr, len);
}
