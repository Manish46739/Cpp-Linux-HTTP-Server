#include "server/response_cache.hpp"

#include <gtest/gtest.h>
#include <thread>

TEST(ResponseCache, StoresAndReturnsEntry) {
    server::ResponseCache cache(16, 60);
    http::HttpResponse response = http::make_response(200, "cached body\n");

    cache.put("GET:/cached", response);
    const auto hit = cache.get("GET:/cached");

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->status_code, 200);
    EXPECT_EQ(hit->body, "cached body\n");
}

TEST(ResponseCache, MissOnUnknownKey) {
    server::ResponseCache cache(16, 60);
    EXPECT_FALSE(cache.get("GET:/missing").has_value());
}

TEST(ResponseCache, DisabledCacheAlwaysMisses) {
    server::ResponseCache cache(0, 60);
    cache.put("GET:/x", http::make_response(200, "x\n"));
    EXPECT_FALSE(cache.get("GET:/x").has_value());
}

TEST(ResponseCache, ThreadSafeUnderConcurrentAccess) {
    server::ResponseCache cache(64, 120);
    http::HttpResponse response = http::make_response(200, "thread-safe\n");

    auto writer = [&]() {
        for (int i = 0; i < 50; ++i) {
            cache.put("GET:/shared", response);
        }
    };

    auto reader = [&]() {
        for (int i = 0; i < 50; ++i) {
            (void)cache.get("GET:/shared");
        }
    };

    std::thread t1(writer);
    std::thread t2(reader);
    std::thread t3(writer);
    std::thread t4(reader);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    const auto hit = cache.get("GET:/shared");
    ASSERT_TRUE(hit.has_value());
}
