#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

namespace logger {

enum class Level { Debug, Info, Warn, Error };

class Logger {
public:
    Logger();
    ~Logger();

    void set_level(Level level) { min_level_ = level; }
    Level level() const { return min_level_; }

    void set_async(bool enabled);
    void shutdown();

    void log(Level level, const std::string& message);

    template <typename... Args>
    void debug(Args&&... args) {
        log(Level::Debug, format(std::forward<Args>(args)...));
    }

    template <typename... Args>
    void info(Args&&... args) {
        log(Level::Info, format(std::forward<Args>(args)...));
    }

    template <typename... Args>
    void warn(Args&&... args) {
        log(Level::Warn, format(std::forward<Args>(args)...));
    }

    template <typename... Args>
    void error(Args&&... args) {
        log(Level::Error, format(std::forward<Args>(args)...));
    }

private:
    template <typename... Args>
    static std::string format(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        return oss.str();
    }

    static const char* level_name(Level level);
    static bool should_log(Level min_level, Level message_level);
    std::string format_line(Level level, const std::string& message) const;

    void enqueue(std::string line);
    void worker_loop();

    Level min_level_ = Level::Info;
    bool async_enabled_ = true;

    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::queue<std::string> queue_;
    std::thread worker_;
    std::atomic<bool> running_{false};
};

Logger& instance();
void init(Level level, bool async = true);
void shutdown();

} // namespace logger
