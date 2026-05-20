#include "server/request_pipeline.hpp"

#include "logger/logger.hpp"

namespace server {

RequestPipeline::RequestPipeline(const config::ServerConfig& config, const router::Router& router)
    : config_(config),
      router_(router),
      rate_limiter_(config.rate_limit_rps, config.rate_limit_burst),
      cache_(config.cache_max_entries, config.cache_ttl_seconds) {}

bool RequestPipeline::is_cacheable(const http::HttpRequest& request,
                                   const http::HttpResponse& response) const {
    if (request.method != "GET" || response.status_code != 200) {
        return false;
    }

    const auto it = response.headers.find("Content-Type");
    if (it == response.headers.end()) {
        return false;
    }

    const std::string& type = it->second;
    return type.rfind("text/", 0) == 0 || type.rfind("application/javascript", 0) == 0 ||
           type.rfind("application/json", 0) == 0 || type.rfind("image/", 0) == 0;
}

http::HttpResponse RequestPipeline::process(const http::HttpRequest& request,
                                             const std::string& client_ip) const {
    // Middleware 1: rate limiting
    if (rate_limiter_.enabled() && !rate_limiter_.allow(client_ip)) {
        logger::instance().warn("Rate limit exceeded for ", client_ip, " ", request.path);
        return http::make_response(429, "Too Many Requests\n");
    }

    // Middleware 2: cache lookup
    if (cache_.enabled() && request.method == "GET") {
        const std::string key = cache_key(request);
        if (const auto cached = cache_.get(key)) {
            logger::instance().debug("Cache hit for ", key);
            http::HttpResponse response = *cached;
            if (http::should_keep_alive(request)) {
                response.headers["Connection"] = "keep-alive";
            } else {
                response.headers["Connection"] = "close";
            }
            return response;
        }
    }

    // Handler: router
    http::HttpResponse response = router_.route(request);

    // Middleware 3: cache store
    if (cache_.enabled() && is_cacheable(request, response)) {
        cache_.put(cache_key(request), response);
        logger::instance().debug("Cache store for ", cache_key(request));
    }

    return response;
}

} // namespace server
