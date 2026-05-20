#pragma once

#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>

namespace server {

// Token-bucket rate limiter keyed by client IP.
class RateLimiter {
public:
    RateLimiter(double requests_per_second, std::size_t burst_size);

    bool allow(const std::string& key);

    bool enabled() const { return requests_per_second_ > 0.0; }

private:
    struct Bucket {
        double tokens = 0.0;
        std::chrono::steady_clock::time_point last_refill =
            std::chrono::steady_clock::now();
    };

    void refill(Bucket& bucket) const;

    double requests_per_second_;
    double burst_size_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Bucket> buckets_;
};

} // namespace server
