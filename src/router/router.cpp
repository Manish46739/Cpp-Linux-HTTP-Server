#include "router/router.hpp"

#include "server/static_file.hpp"

namespace router {

void Router::add_route(const std::string& method, const std::string& path, Handler handler) {
    routes_[RouteKey{method, path}] = std::move(handler);
}

void Router::set_fallback(Handler handler) {
    fallback_ = std::move(handler);
}

http::HttpResponse Router::route(const http::HttpRequest& request) const {
    const auto it = routes_.find(RouteKey{request.method, request.path});
    if (it != routes_.end()) {
        return it->second(request);
    }

    if (fallback_) {
        return (*fallback_)(request);
    }

    return http::make_response(404, "Not Found\n");
}

Router make_default_router(const config::ServerConfig& config) {
    Router router;
    const std::string static_root = config.static_root;

    router.add_route("GET", "/health", [](const http::HttpRequest&) {
        return http::make_response(200, "ok\n");
    });

    router.add_route("GET", "/", [static_root](const http::HttpRequest& request) {
        return server::serve_static(static_root, request.path);
    });

    router.set_fallback([static_root](const http::HttpRequest& request) {
        if (request.method != "GET") {
            return http::make_response(405, "Method Not Allowed\n");
        }
        return server::serve_static(static_root, request.path);
    });

    return router;
}

} // namespace router
