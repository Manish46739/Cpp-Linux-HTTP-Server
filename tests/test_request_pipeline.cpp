#include "config/server_config.hpp"
#include "logger/logger.hpp"
#include "router/router.hpp"
#include "server/request_pipeline.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace {

config::ServerConfig make_pipeline_config() {
    config::ServerConfig cfg;
    cfg.static_root = "static";
    cfg.rate_limit_rps = 1000.0;
    cfg.rate_limit_burst = 1000;
    cfg.cache_max_entries = 32;
    cfg.cache_ttl_seconds = 60;
    return cfg;
}

} // namespace

class PipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger::init(logger::Level::Error, false);
        cfg_ = make_pipeline_config();
        router_ = std::make_unique<router::Router>(router::make_default_router(cfg_));
        pipeline_ = std::make_unique<server::RequestPipeline>(cfg_, *router_);
    }

    config::ServerConfig cfg_;
    std::unique_ptr<router::Router> router_;
    std::unique_ptr<server::RequestPipeline> pipeline_;
};

TEST_F(PipelineTest, RoutesHealthRequest) {
    http::HttpRequest request;
    request.method = "GET";
    request.path = "/health";
    request.version = "HTTP/1.1";

    const http::HttpResponse response = pipeline_->process(request, "127.0.0.1");
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.body, "ok\n");
}

TEST_F(PipelineTest, RateLimitReturns429) {
    config::ServerConfig strict_cfg = make_pipeline_config();
    strict_cfg.rate_limit_rps = 1.0;
    strict_cfg.rate_limit_burst = 1.0;
    strict_cfg.cache_max_entries = 0;

    const router::Router router = router::make_default_router(strict_cfg);
    const server::RequestPipeline strict_pipeline(strict_cfg, router);

    http::HttpRequest request;
    request.method = "GET";
    request.path = "/health";
    request.version = "HTTP/1.1";

    EXPECT_EQ(strict_pipeline.process(request, "10.0.0.1").status_code, 200);
    EXPECT_EQ(strict_pipeline.process(request, "10.0.0.1").status_code, 429);
}

TEST_F(PipelineTest, CacheHitOnRepeatedGet) {
    http::HttpRequest request;
    request.method = "GET";
    request.path = "/health";
    request.version = "HTTP/1.1";

    const http::HttpResponse first = pipeline_->process(request, "127.0.0.1");
    const http::HttpResponse second = pipeline_->process(request, "127.0.0.1");

    EXPECT_EQ(first.status_code, 200);
    EXPECT_EQ(second.status_code, 200);
    EXPECT_EQ(second.body, first.body);
}
