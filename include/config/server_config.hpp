#pragma once

#include "logger/logger.hpp"

#include <cstdint>
#include <string>

namespace config {

enum class IoModel { ThreadPool, Epoll };

struct ServerConfig {
    uint16_t port = 8080;
    std::size_t thread_count = 4;
    std::string static_root = "static";
    logger::Level log_level = logger::Level::Info;
    IoModel io_model = IoModel::Epoll;

    bool async_logging = true;
    double rate_limit_rps = 50.0;
    std::size_t rate_limit_burst = 100;
    std::size_t cache_max_entries = 128;
    std::size_t cache_ttl_seconds = 60;
    std::size_t request_timeout_seconds = 30;

    static ServerConfig load(int argc, char** argv);
};

} // namespace config
