#include "http/http_request.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

namespace http {
namespace {

std::string trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())) != 0) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())) != 0) {
        s.remove_suffix(1);
    }
    return std::string(s);
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

size_t find_header_body_split(const std::string& raw) {
    const auto pos = raw.find("\r\n\r\n");
    if (pos != std::string::npos) {
        return pos;
    }
    return raw.find("\n\n");
}

bool parse_request_line(const std::string& line, HttpRequest& out) {
    std::istringstream iss(line);
    if (!(iss >> out.method >> out.path >> out.version)) {
        return false;
    }
    return !out.method.empty() && !out.path.empty() && !out.version.empty();
}

bool parse_headers(const std::string& header_block, HttpRequest& out) {
    std::istringstream stream(header_block);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            return false;
        }

        const std::string key = to_lower(trim(std::string_view(line).substr(0, colon)));
        const std::string value = trim(std::string_view(line).substr(colon + 1));
        if (key.empty()) {
            return false;
        }
        out.headers[key] = value;
    }
    return true;
}

bool parse_content_length(const std::string& header_part, std::size_t& out_length) {
    std::istringstream stream(header_part);
    std::string line;
    bool past_request_line = false;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!past_request_line) {
            past_request_line = true;
            continue;
        }
        if (line.empty()) {
            continue;
        }

        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        const std::string key = to_lower(trim(std::string_view(line).substr(0, colon)));
        if (key == "content-length") {
            const std::string value = trim(std::string_view(line).substr(colon + 1));
            try {
                out_length = static_cast<std::size_t>(std::stoul(value));
                return true;
            } catch (const std::exception&) {
                return false;
            }
        }
    }

    out_length = 0;
    return true;
}

} // namespace

std::optional<std::size_t> complete_request_size(const std::string& buffer) {
    const size_t split = find_header_body_split(buffer);
    if (split == std::string::npos) {
        return std::nullopt;
    }

    const size_t delim_len = (buffer.compare(split, 4, "\r\n\r\n") == 0) ? 4 : 2;
    const std::string header_part = buffer.substr(0, split);

    std::size_t content_length = 0;
    if (!parse_content_length(header_part, content_length)) {
        return std::nullopt;
    }

    const std::size_t total = split + delim_len + content_length;
    if (buffer.size() < total) {
        return std::nullopt;
    }

    return total;
}

bool should_keep_alive(const HttpRequest& request) {
    const auto it = request.headers.find("connection");
    if (it != request.headers.end()) {
        const std::string value = to_lower(it->second);
        if (value.find("close") != std::string::npos) {
            return false;
        }
        if (value.find("keep-alive") != std::string::npos) {
            return true;
        }
    }

    return request.version.size() >= 8 && request.version.rfind("HTTP/1.1") == 0;
}

std::optional<HttpRequest> parse_http_request(const std::string& raw) {
    if (raw.empty()) {
        return std::nullopt;
    }

    const size_t split = find_header_body_split(raw);
    const std::string header_part = (split == std::string::npos) ? raw : raw.substr(0, split);
    const std::string body_part =
        (split == std::string::npos) ? std::string{} : raw.substr(split + (raw.compare(split, 4, "\r\n\r\n") == 0 ? 4 : 2));

    const auto first_line_end = header_part.find('\n');
    if (first_line_end == std::string::npos) {
        return std::nullopt;
    }

    std::string first_line = header_part.substr(0, first_line_end);
    if (!first_line.empty() && first_line.back() == '\r') {
        first_line.pop_back();
    }

    HttpRequest request;
    if (!parse_request_line(first_line, request)) {
        return std::nullopt;
    }

    const std::string remaining_headers = header_part.substr(first_line_end + 1);
    if (!parse_headers(remaining_headers, request)) {
        return std::nullopt;
    }

    request.body = body_part;
    return request;
}

} // namespace http
