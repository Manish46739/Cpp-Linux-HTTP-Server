#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>

namespace threadpool {

// Thread-safe FIFO queue of callables. Blocks workers until a task is available.
class TaskQueue {
public:
    void enqueue(std::function<void()> task);
    std::optional<std::function<void()>> dequeue();

    void shutdown();
    bool is_shutdown() const;

    std::size_t size() const;

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> tasks_;
    bool shutdown_ = false;
};

} // namespace threadpool
