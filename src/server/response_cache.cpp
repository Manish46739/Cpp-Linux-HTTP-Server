#include "server/response_cache.hpp"

namespace server {

ResponseCache::ResponseCache(const std::size_t max_entries, const std::size_t ttl_seconds)
    : max_entries_(max_entries), ttl_seconds_(ttl_seconds) {}

std::string cache_key(const http::HttpRequest& request) {
    return request.method + ":" + request.path;
}

void ResponseCache::evict_expired_unsafe() const {
    const auto now = std::chrono::steady_clock::now();
    for (auto it = entries_.begin(); it != entries_.end();) {
        if (it->second.first.expires_at <= now) {
            lru_order_.erase(it->second.second);
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

void ResponseCache::evict_lru_unsafe() {
    if (lru_order_.empty()) {
        return;
    }
    const std::string& key = lru_order_.back();
    entries_.erase(key);
    lru_order_.pop_back();
}

std::optional<http::HttpResponse> ResponseCache::get(const std::string& key) const {
    if (!enabled()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    evict_expired_unsafe();

    const auto it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }

    if (it->second.first.expires_at <= std::chrono::steady_clock::now()) {
        lru_order_.erase(it->second.second);
        entries_.erase(it);
        return std::nullopt;
    }

    lru_order_.splice(lru_order_.begin(), lru_order_, it->second.second);
    return it->second.first.response;
}

void ResponseCache::put(const std::string& key, const http::HttpResponse& response) {
    if (!enabled()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    evict_expired_unsafe();

    const auto existing = entries_.find(key);
    if (existing != entries_.end()) {
        lru_order_.erase(existing->second.second);
        entries_.erase(existing);
    }

    while (entries_.size() >= max_entries_) {
        evict_lru_unsafe();
    }

    const auto expires_at =
        std::chrono::steady_clock::now() + std::chrono::seconds(static_cast<int>(ttl_seconds_));

    lru_order_.push_front(key);
    entries_[key] = std::make_pair(CacheEntry{response, expires_at}, lru_order_.begin());
}

} // namespace server
