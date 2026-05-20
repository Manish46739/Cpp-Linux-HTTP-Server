#include "logger/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace logger {
namespace {

Logger g_logger;

} // namespace

Logger::Logger() = default;

Logger::~Logger() {
    shutdown();
}

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

std::string Logger::format_line(const Level level, const std::string& message) const {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);

    std::ostringstream line;
    line << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    line << " [" << level_name(level) << "] " << message;
    return line.str();
}

void Logger::set_async(const bool enabled) {
    async_enabled_ = enabled;
    if (!async_enabled_) {
        return;
    }

    bool expected = false;
    if (running_.compare_exchange_strong(expected, true)) {
        worker_ = std::thread([this] { worker_loop(); });
    }
}

void Logger::shutdown() {
    if (!running_.exchange(false)) {
        return;
    }

    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }

    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!queue_.empty()) {
        std::cout << queue_.front() << '\n';
        queue_.pop();
    }
}

void Logger::enqueue(std::string line) {
    if (async_enabled_) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            queue_.push(std::move(line));
        }
        cv_.notify_one();
        return;
    }

    std::cout << line << '\n';
}

void Logger::worker_loop() {
    while (running_.load()) {
        std::string line;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { return !running_.load() || !queue_.empty(); });

            if (!queue_.empty()) {
                line = std::move(queue_.front());
                queue_.pop();
            }
        }

        if (!line.empty()) {
            std::cout << line << '\n';
        }
    }
}

void Logger::log(const Level level, const std::string& message) {
    if (!should_log(min_level_, level)) {
        return;
    }

    enqueue(format_line(level, message));
}

Logger& instance() {
    return g_logger;
}

void init(const Level level, const bool async) {
    g_logger.set_level(level);
    g_logger.set_async(async);
}

void shutdown() {
    g_logger.shutdown();
}

} // namespace logger
