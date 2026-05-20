#pragma once

#include <mutex>
#include <sstream>
#include <string>

namespace logger {

enum class Level { Debug, Info, Warn, Error };

class Logger {
public:
    void set_level(Level level) { min_level_ = level; }
    Level level() const { return min_level_; }

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

    std::mutex mutex_;
    Level min_level_ = Level::Info;
};

Logger& instance();
void init(Level level);

} // namespace logger
