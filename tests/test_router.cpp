#include "config/server_config.hpp"
#include "router/router.hpp"

#include <gtest/gtest.h>

namespace {

config::ServerConfig test_config() {
    config::ServerConfig cfg;
    cfg.static_root = "static";
    cfg.rate_limit_rps = 0;
    cfg.cache_max_entries = 0;
    return cfg;
}

} // namespace

TEST(Router, HealthEndpoint) {
    const router::Router router = router::make_default_router(test_config());
    http::HttpRequest request;
    request.method = "GET";
    request.path = "/health";
    request.version = "HTTP/1.1";

    const http::HttpResponse response = router.route(request);
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.body, "ok\n");
}

TEST(Router, RootServesIndexHtml) {
    const router::Router router = router::make_default_router(test_config());
    http::HttpRequest request;
    request.method = "GET";
    request.path = "/";
    request.version = "HTTP/1.1";

    const http::HttpResponse response = router.route(request);
    EXPECT_EQ(response.status_code, 200);
    EXPECT_NE(response.body.find("C++ HTTP Server"), std::string::npos);
}

TEST(Router, UnknownPathReturns404) {
    const router::Router router = router::make_default_router(test_config());
    http::HttpRequest request;
    request.method = "GET";
    request.path = "/does-not-exist-xyz";
    request.version = "HTTP/1.1";

    const http::HttpResponse response = router.route(request);
    EXPECT_EQ(response.status_code, 404);
}

TEST(Router, PostToUnknownReturns405) {
    const router::Router router = router::make_default_router(test_config());
    http::HttpRequest request;
    request.method = "POST";
    request.path = "/unknown";
    request.version = "HTTP/1.1";

    const http::HttpResponse response = router.route(request);
    EXPECT_EQ(response.status_code, 405);
}
