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

// Returns total byte length of one complete request, or nullopt if more data is needed.
std::optional<std::size_t> complete_request_size(const std::string& buffer);

bool should_keep_alive(const HttpRequest& request);

} // namespace http
