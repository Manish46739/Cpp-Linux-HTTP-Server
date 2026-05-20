#pragma once

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace utils {

// Serializes std::cout writes from multiple threads.
inline void log_line(const std::string& message) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    std::cout << message << std::flush;
    if (message.empty() || message.back() != '\n') {
        std::cout << '\n';
    }
}

template <typename... Args>
void log(Args&&... args) {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    log_line(oss.str());
}

} // namespace utils
