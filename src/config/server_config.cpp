#include "config/server_config.hpp"

#include <cstdlib>
#include <stdexcept>

namespace config {
namespace {

std::string env_or(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    return value;
}

uint16_t parse_port(const std::string& value) {
    const int port = std::stoi(value);
    if (port < 1 || port > 65535) {
        throw std::invalid_argument("port out of range");
    }
    return static_cast<uint16_t>(port);
}

std::size_t parse_thread_count(const std::string& value) {
    const int count = std::stoi(value);
    if (count < 1) {
        return 1;
    }
    return static_cast<std::size_t>(count);
}

IoModel parse_io_model(const std::string& value) {
    if (value == "threadpool" || value == "threads" || value == "THREADPOOL") {
        return IoModel::ThreadPool;
    }
    return IoModel::Epoll;
}

bool parse_bool(const std::string& value) {
    return value == "1" || value == "true" || value == "TRUE" || value == "yes" || value == "YES";
}

double parse_rate_limit_rps(const std::string& value) {
    const double rps = std::stod(value);
    if (rps < 0.0) {
        return 0.0;
    }
    return rps;
}

std::size_t parse_size_t(const std::string& value) {
    const int n = std::stoi(value);
    if (n < 0) {
        return 0;
    }
    return static_cast<std::size_t>(n);
}

logger::Level parse_log_level(const std::string& value) {
    if (value == "DEBUG" || value == "debug") {
        return logger::Level::Debug;
    }
    if (value == "WARN" || value == "warn") {
        return logger::Level::Warn;
    }
    if (value == "ERROR" || value == "error") {
        return logger::Level::Error;
    }
    return logger::Level::Info;
}

} // namespace

ServerConfig ServerConfig::load(int argc, char** argv) {
    ServerConfig cfg;

    try {
        cfg.port = parse_port(env_or("SERVER_PORT", "8080"));
        cfg.thread_count = parse_thread_count(env_or("SERVER_THREADS", "4"));
        cfg.static_root = env_or("SERVER_STATIC_ROOT", "static");
        cfg.log_level = parse_log_level(env_or("SERVER_LOG_LEVEL", "INFO"));
        cfg.io_model = parse_io_model(env_or("SERVER_IO_MODEL", "epoll"));
        cfg.async_logging = parse_bool(env_or("SERVER_ASYNC_LOG", "true"));
        cfg.rate_limit_rps = parse_rate_limit_rps(env_or("SERVER_RATE_LIMIT_RPS", "50"));
        cfg.rate_limit_burst = parse_size_t(env_or("SERVER_RATE_LIMIT_BURST", "100"));
        cfg.cache_max_entries = parse_size_t(env_or("SERVER_CACHE_MAX_ENTRIES", "128"));
        cfg.cache_ttl_seconds = parse_size_t(env_or("SERVER_CACHE_TTL_SECONDS", "60"));
        cfg.request_timeout_seconds = parse_size_t(env_or("SERVER_REQUEST_TIMEOUT_SECONDS", "30"));
    } catch (const std::exception&) {
        throw std::invalid_argument("invalid environment configuration");
    }

    if (argc >= 2) {
        cfg.port = parse_port(argv[1]);
    }
    if (argc >= 3) {
        cfg.thread_count = parse_thread_count(argv[2]);
    }

    return cfg;
}

} // namespace config
