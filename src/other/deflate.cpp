#include "other/deflate.h"

std::vector<uint8_t> compressData(const std::vector<uint8_t>& data) {
    z_stream zs = {};
    if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) return {};

    zs.next_in = const_cast<Bytef *>(data.data());
    zs.avail_in = data.size();

    int ret;
    std::vector<uint8_t> out;
    uint8_t temp[32768];

    do {
        zs.next_out = temp;
        zs.avail_out = sizeof(temp);
        ret = deflate(&zs, Z_FINISH);
        out.insert(out.end(), temp, temp + (sizeof(temp) - zs.avail_out));
    } while (ret == Z_OK);

    deflateEnd(&zs);
    return out;
}