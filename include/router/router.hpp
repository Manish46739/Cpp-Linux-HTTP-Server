#pragma once

#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace router {

using Handler = std::function<http::HttpResponse(const http::HttpRequest&)>;

class Router {
public:
    void add_route(const std::string& method, const std::string& path, Handler handler);

    http::HttpResponse route(const http::HttpRequest& request) const;

private:
    struct RouteKey {
        std::string method;
        std::string path;

        bool operator==(const RouteKey& other) const {
            return method == other.method && path == other.path;
        }
    };

    struct RouteKeyHash {
        std::size_t operator()(const RouteKey& key) const {
            std::size_t h1 = std::hash<std::string>{}(key.method);
            std::size_t h2 = std::hash<std::string>{}(key.path);
            return h1 ^ (h2 << 1);
        }
    };

    std::unordered_map<RouteKey, Handler, RouteKeyHash> routes_;
};

Router make_default_router();

} // namespace router
