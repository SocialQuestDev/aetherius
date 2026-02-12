#include "auth/MojangAuth.h"
#include "network/Http.h"
#include "utils/StringUtilities.h"
#include "console/Logger.h"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <random>
#include <boost/multiprecision/cpp_int.hpp>

using json = nlohmann::json;

MojangAuth::MojangAuth() = default;
MojangAuth::~MojangAuth() = default;

std::string MojangAuth::minecraftSha1ToHex(const unsigned char* hash) {
    using boost::multiprecision::cpp_int;

    cpp_int num = 0;

    for (int i = 0; i < 20; i++) {
        num <<= 8;
        num |= hash[i];
    }

    if (hash[0] & 0x80) { // if negative
        auto max = cpp_int(1);
        max <<= 160;
        num -= max;
    }

    std::stringstream ss;
    ss << std::hex << num;

    return ss.str();
}

std::string MojangAuth::calculateServerHash(
    const std::string& serverId,
    const std::vector<uint8_t>& secret,
    const std::vector<uint8_t>& pubKey
) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha1();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, serverId.data(), serverId.size());
    EVP_DigestUpdate(mdctx, secret.data(), secret.size());
    EVP_DigestUpdate(mdctx, pubKey.data(), pubKey.size());
    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    EVP_MD_CTX_free(mdctx);

    return minecraftSha1ToHex(hash);
}

std::vector<uint8_t> MojangAuth::generateVerifyToken() {
    std::vector<uint8_t> token(4);

    std::random_device rd;
    for (auto& b : token)
        b = static_cast<uint8_t>(rd());

    return token;
}

std::string MojangAuth::generateUuidUrl(const std::string& nickname) {
    return "https://api.mojang.com/users/profiles/minecraft/" + nickname;
}

std::string MojangAuth::generateProfileUrl(const std::string& uuid) {
    return "https://sessionserver.mojang.com/session/minecraft/profile/" + uuid;
}

Auth::PlayerProfile MojangAuth::getPlayerProfile(const std::string& nickname) {
    std::string url = generateUuidUrl(nickname);

    url = trim(url);

    LOG_DEBUG("UUID url: " + url);

    HttpResponse uuidResponse = http::get(url);

    if (uuidResponse.status != 200) {
        LOG_ERROR("Mojang response status: " + std::to_string(uuidResponse.status) + ", with body: " + uuidResponse.body);
        LOG_WARN("Generating offline UUID");
        const UUID uuid = get_offline_UUID_128(nickname);

        return {uuid, ""};
    }

    json uuidJson = json::parse(uuidResponse.body);

    const std::string uuidRaw = uuidJson["id"];

    const UUID uuid = string_to_uuid(uuidRaw);

    std::string profileUrl = generateProfileUrl(uuidRaw);

    profileUrl = trim(profileUrl);

    LOG_DEBUG("Profile URL:" + profileUrl);

    HttpResponse profileResponse = http::get(profileUrl);

    if (profileResponse.status != 200) {
        LOG_WARN("Mojang profile response status: " + std::to_string(profileResponse.status) + ", body: " + profileResponse.body);
        LOG_WARN("Using empty skin");

        return {uuid, ""};
    }

    json profileJson = json::parse(profileResponse.body);

    const std::string textures = profileJson["properties"][0]["value"];

    return {uuid, textures};
}
