#pragma once

#include "http/http_response.hpp"

#include <chrono>
#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace server {

// Thread-safe LRU cache with TTL for HTTP responses.
class ResponseCache {
public:
    ResponseCache(std::size_t max_entries, std::size_t ttl_seconds);

    std::optional<http::HttpResponse> get(const std::string& key) const;
    void put(const std::string& key, const http::HttpResponse& response);

    bool enabled() const { return max_entries_ > 0 && ttl_seconds_ > 0; }

private:
    struct CacheEntry {
        http::HttpResponse response;
        std::chrono::steady_clock::time_point expires_at;
    };

    void evict_expired_unsafe() const;
    void evict_lru_unsafe();

    std::size_t max_entries_;
    std::size_t ttl_seconds_;

    mutable std::mutex mutex_;
    mutable std::list<std::string> lru_order_;
    mutable std::unordered_map<std::string, std::pair<CacheEntry, std::list<std::string>::iterator>>
        entries_;
};

std::string cache_key(const http::HttpRequest& request);

} // namespace server
