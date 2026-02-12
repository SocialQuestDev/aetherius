#include "utils/VectorUtilities.h"
#include <cstring>

bool vectors_equal(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    if (a.size() != b.size()) return false;
    if (a.empty()) return true;
    return std::memcmp(a.data(), b.data(), a.size()) == 0;
}