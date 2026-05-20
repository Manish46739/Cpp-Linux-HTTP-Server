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
