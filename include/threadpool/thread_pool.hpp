#pragma once

#include "threadpool/task_queue.hpp"

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

namespace threadpool {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t num_threads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(std::function<void()> task);
    void shutdown();

    std::size_t size() const { return workers_.size(); }

private:
    void worker_loop();

    TaskQueue queue_;
    std::vector<std::thread> workers_;
};

} // namespace threadpool
