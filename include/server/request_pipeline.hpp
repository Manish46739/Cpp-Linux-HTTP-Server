#pragma once

#include "config/server_config.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "router/router.hpp"
#include "server/rate_limiter.hpp"
#include "server/response_cache.hpp"

#include <string>

namespace server {

// Middleware-style request processing: rate limit -> cache -> router -> cache store.
class RequestPipeline {
public:
    RequestPipeline(const config::ServerConfig& config, const router::Router& router);

    http::HttpResponse process(const http::HttpRequest& request,
                               const std::string& client_ip);

private:
    bool is_cacheable(const http::HttpRequest& request, const http::HttpResponse& response) const;

    const config::ServerConfig& config_;
    const router::Router& router_;
    RateLimiter rate_limiter_;
    ResponseCache cache_;
};

} // namespace server
