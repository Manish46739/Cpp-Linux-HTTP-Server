#include "server/static_file.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace server {
namespace {

namespace fs = std::filesystem;

const std::unordered_map<std::string, const char*> kMimeTypes = {
    {".html", "text/html; charset=utf-8"},
    {".htm", "text/html; charset=utf-8"},
    {".css", "text/css; charset=utf-8"},
    {".js", "application/javascript; charset=utf-8"},
    {".json", "application/json; charset=utf-8"},
    {".txt", "text/plain; charset=utf-8"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
};

bool contains_path_traversal(const std::string& path) {
    return path.find("..") != std::string::npos;
}

std::string mime_type_for(const fs::path& path) {
    const std::string ext = path.extension().string();
    const auto it = kMimeTypes.find(ext);
    if (it != kMimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

std::optional<fs::path> resolve_file_path(const std::string& static_root, const std::string& url_path) {
    if (contains_path_traversal(url_path)) {
        return std::nullopt;
    }

    std::string relative = url_path;
    if (relative.empty() || relative.front() != '/') {
        return std::nullopt;
    }
    relative.erase(relative.begin());

    if (relative.empty()) {
        relative = "index.html";
    }

    const fs::path root = fs::weakly_canonical(static_root);
    const fs::path candidate = root / relative;
    const fs::path resolved = fs::weakly_canonical(candidate);

    const std::string root_str = root.string();
    const std::string resolved_str = resolved.string();
    if (resolved_str.size() < root_str.size() ||
        resolved_str.compare(0, root_str.size(), root_str) != 0) {
        return std::nullopt;
    }

    if (!fs::exists(resolved) || !fs::is_regular_file(resolved)) {
        return std::nullopt;
    }

    return resolved;
}

std::optional<std::string> read_file_bytes(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace

http::HttpResponse serve_static(const std::string& static_root, const std::string& url_path) {
    const auto file_path = resolve_file_path(static_root, url_path);
    if (!file_path) {
        if (contains_path_traversal(url_path)) {
            return http::make_response(403, "Forbidden\n");
        }
        return http::make_response(404, "Not Found\n");
    }

    const auto body = read_file_bytes(*file_path);
    if (!body) {
        return http::make_response(500, "Internal Server Error\n");
    }

    return http::make_response(200, *body, mime_type_for(*file_path));
}

} // namespace server
