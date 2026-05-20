#include "server/rate_limiter.hpp"

#include <gtest/gtest.h>

TEST(RateLimiter, DisabledAllowsAll) {
    server::RateLimiter limiter(0.0, 10);
    EXPECT_FALSE(limiter.enabled());
    for (int i = 0; i < 20; ++i) {
        EXPECT_TRUE(limiter.allow("127.0.0.1"));
    }
}

TEST(RateLimiter, EnforcesBurst) {
    server::RateLimiter limiter(1.0, 3.0);
    EXPECT_TRUE(limiter.allow("client-a"));
    EXPECT_TRUE(limiter.allow("client-a"));
    EXPECT_TRUE(limiter.allow("client-a"));
    EXPECT_FALSE(limiter.allow("client-a"));
}

TEST(RateLimiter, KeysAreIndependent) {
    server::RateLimiter limiter(1.0, 1.0);
    EXPECT_TRUE(limiter.allow("client-a"));
    EXPECT_FALSE(limiter.allow("client-a"));
    EXPECT_TRUE(limiter.allow("client-b"));
}
