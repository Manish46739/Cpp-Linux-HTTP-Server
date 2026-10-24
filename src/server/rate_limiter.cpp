#include "server/rate_limiter.hpp"

#include <algorithm>

namespace server {

RateLimiter::RateLimiter(const double requests_per_second, const std::size_t burst_size)
    : requests_per_second_(requests_per_second),
      burst_size_(static_cast<double>(burst_size > 0 ? burst_size : 1)) {}

void RateLimiter::refill(Bucket& bucket) const {
    const auto now = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = now - bucket.last_refill;
    bucket.last_refill = now;

    bucket.tokens = std::min(
        burst_size_, bucket.tokens + elapsed.count() * requests_per_second_);
}

bool RateLimiter::allow(const std::string& key) {
    if (!enabled()) {
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::chrono::steady_clock::now();
    auto bucket_result = buckets_.try_emplace(key, Bucket{burst_size_, now});
    Bucket& bucket = bucket_result.first->second;
    refill(bucket);

    if (bucket.tokens >= 1.0) {
        bucket.tokens -= 1.0;
        return true;
    }

    return false;
}

} // namespace server
