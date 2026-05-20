#include "router/router.hpp"

namespace router {

void Router::add_route(const std::string& method, const std::string& path, Handler handler) {
    routes_[RouteKey{method, path}] = std::move(handler);
}

http::HttpResponse Router::route(const http::HttpRequest& request) const {
    const auto it = routes_.find(RouteKey{request.method, request.path});
    if (it == routes_.end()) {
        return http::make_response(404, "Not Found\n");
    }
    return it->second(request);
}

Router make_default_router() {
    Router router;

    router.add_route("GET", "/", [](const http::HttpRequest&) {
        return http::make_response(200, "Hello from C++ HTTP server\n");
    });

    router.add_route("GET", "/health", [](const http::HttpRequest&) {
        return http::make_response(200, "ok\n");
    });

    return router;
}

} // namespace router
