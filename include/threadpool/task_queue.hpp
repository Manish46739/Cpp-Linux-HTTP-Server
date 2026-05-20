#pragma once

#include "threadpool/task.hpp"

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>

namespace threadpool {

// Thread-safe FIFO queue of move-only tasks.
class TaskQueue {
public:
    void enqueue(std::unique_ptr<ITask> task);

    template <typename F>
    void enqueue(F&& f) {
        enqueue(make_task(std::forward<F>(f)));
    }

    std::unique_ptr<ITask> dequeue();

    void shutdown();
    bool is_shutdown() const;

    std::size_t size() const;

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::unique_ptr<ITask>> tasks_;
    bool shutdown_ = false;
};

} // namespace threadpool
