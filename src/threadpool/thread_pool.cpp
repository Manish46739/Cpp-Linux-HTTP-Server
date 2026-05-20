#include "threadpool/thread_pool.hpp"

namespace threadpool {

ThreadPool::ThreadPool(std::size_t num_threads) {
    if (num_threads == 0) {
        num_threads = 1;
    }
    workers_.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::submit(std::function<void()> task) {
    queue_.enqueue(std::move(task));
}

void ThreadPool::shutdown() {
    queue_.shutdown();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::optional<std::function<void()>> task = queue_.dequeue();
        if (!task) {
            return;
        }
        (*task)();
    }
}

} // namespace threadpool
