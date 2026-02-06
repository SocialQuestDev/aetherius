#include "../../include/utility/StringUtilities.h"
#include <algorithm>
#include <cctype>

std::string trim(const std::string& s)
{
    auto wsfront = std::find_if_not(s.begin(), s.end(),
        [](unsigned char c){return std::isspace(c);});
    auto wsback = std::find_if_not(s.rbegin(), s.rend(),
        [](unsigned char c){return std::isspace(c);}).base();
    return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
}