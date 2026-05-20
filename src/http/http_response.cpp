#include "http/http_response.hpp"

#include <sstream>
#include <unordered_map>

namespace http {
namespace {

const std::unordered_map<int, const char*> kStatusTexts = {
    {200, "OK"},
    {400, "Bad Request"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {429, "Too Many Requests"},
    {500, "Internal Server Error"},
};

} // namespace

std::string status_text(int status_code) {
    const auto it = kStatusTexts.find(status_code);
    if (it != kStatusTexts.end()) {
        return it->second;
    }
    return "Unknown";
}

HttpResponse make_response(int status_code,
                           const std::string& body,
                           const std::string& content_type,
                           const bool keep_alive) {
    HttpResponse response;
    response.status_code = status_code;
    response.status_message = status_text(status_code);
    response.body = body;
    response.headers["Content-Type"] = content_type;
    response.headers["Content-Length"] = std::to_string(body.size());
    response.headers["Connection"] = keep_alive ? "keep-alive" : "close";
    return response;
}

std::string serialize(const HttpResponse& response) {
    std::ostringstream out;
    out << "HTTP/1.1 " << response.status_code << ' ' << response.status_message << "\r\n";

    for (const auto& [key, value] : response.headers) {
        out << key << ": " << value << "\r\n";
    }

    out << "\r\n";
    out << response.body;
    return out.str();
}

} // namespace http
