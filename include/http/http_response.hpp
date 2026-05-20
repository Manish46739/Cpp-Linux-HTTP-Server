#pragma once

#include <map>
#include <string>

namespace http {

struct HttpResponse {
    int status_code = 200;
    std::string status_message = "OK";
    std::map<std::string, std::string> headers;
    std::string body;
};

std::string status_text(int status_code);

HttpResponse make_response(int status_code,
                           const std::string& body,
                           const std::string& content_type = "text/plain; charset=utf-8",
                           bool keep_alive = false);

// Serializes to wire format (HTTP/1.1, CRLF).
std::string serialize(const HttpResponse& response);

} // namespace http
