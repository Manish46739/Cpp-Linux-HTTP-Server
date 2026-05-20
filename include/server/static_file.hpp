#pragma once

#include "http/http_response.hpp"

#include <string>

namespace server {

// Serves a file under static_root for GET requests. Returns 404/403 on failure.
http::HttpResponse serve_static(const std::string& static_root, const std::string& url_path);

} // namespace server
