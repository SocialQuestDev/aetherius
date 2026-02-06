#include "../../include/auth/MojangAuthHelper.h"
#include "../../include/Server.h"
#include "../../include/network/Http.h"
#include "../../include/utility/StringUtilities.h"
#include <nlohmann/json.hpp>

#include <openssl/sha.h>
#include <random>

#include "../../include/Logger.h"

using json = nlohmann::json;

std::string minecraft_sha1_to_hex(const unsigned char* hash) {
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

std::string calc_server_hash(
    const std::string& serverId,
    const std::vector<uint8_t>& secret,
    const std::vector<uint8_t>& pubKey
) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);

    SHA1_Update(&ctx, serverId.data(), serverId.size());
    SHA1_Update(&ctx, secret.data(), secret.size());
    SHA1_Update(&ctx, pubKey.data(), pubKey.size());

    unsigned char hash[20];
    SHA1_Final(hash, &ctx);

    return minecraft_sha1_to_hex(hash);
}

std::vector<uint8_t> auth::generate_verify_token() {
    std::vector<uint8_t> token(4);

    std::random_device rd;
    for (auto& b : token)
        b = rd();

    return token;
}

std::string auth::generate_uuid_url(const std::string& nickname) {
    return "https://api.mojang.com/users/profiles/minecraft/" + nickname;
}

std::string auth::generate_profile_url(const std::string& uuid) {
    return "https://sessionserver.mojang.com/session/minecraft/profile/" + uuid;
}

PlayerData auth::get_uuid(const std::string& nickname) {
    std::string url = generate_uuid_url(nickname);

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

    std::string profileUrl = generate_profile_url(uuidRaw);

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