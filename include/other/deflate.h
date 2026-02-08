#pragma once
#include <vector>
#include <zlib.h>
#include <cstdint>

std::vector<uint8_t> compressData(const std::vector<uint8_t>& data);