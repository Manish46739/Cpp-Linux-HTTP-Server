#include "logger/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace logger {
namespace {

Logger g_logger;

} // namespace

const char* Logger::level_name(const Level level) {
    switch (level) {
    case Level::Debug:
        return "DEBUG";
    case Level::Info:
        return "INFO";
    case Level::Warn:
        return "WARN";
    case Level::Error:
        return "ERROR";
    }
    return "INFO";
}

bool Logger::should_log(const Level min_level, const Level message_level) {
    return static_cast<int>(message_level) >= static_cast<int>(min_level);
}

void Logger::log(const Level level, const std::string& message) {
    if (!should_log(min_level_, level)) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);

    std::ostringstream line;
    line << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    line << " [" << level_name(level) << "] " << message;

    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << line.str() << '\n';
}

Logger& instance() {
    return g_logger;
}

void init(const Level level) {
    g_logger.set_level(level);
}

} // namespace logger
