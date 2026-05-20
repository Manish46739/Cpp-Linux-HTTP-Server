#pragma once

#include <map>
#include <optional>
#include <string>

namespace http {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

// Returns nullopt if the request is malformed or incomplete.
std::optional<HttpRequest> parse_http_request(const std::string& raw);

} // namespace http
