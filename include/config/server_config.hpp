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

    static ServerConfig load(int argc, char** argv);
};

} // namespace config
