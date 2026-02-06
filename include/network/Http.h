#pragma once
#include <string>

struct HttpResponse
{
    int status = 0;
    std::string headers;
    std::string body;
};

namespace http {
    // supports https!!
    HttpResponse get(const std::string& url);
}