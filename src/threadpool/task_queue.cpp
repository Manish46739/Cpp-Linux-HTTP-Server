#include "threadpool/task_queue.hpp"

namespace threadpool {

void TaskQueue::enqueue(std::unique_ptr<ITask> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            return;
        }
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

std::unique_ptr<ITask> TaskQueue::dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return shutdown_ || !tasks_.empty(); });

    if (tasks_.empty()) {
        return nullptr;
    }

    std::unique_ptr<ITask> task = std::move(tasks_.front());
    tasks_.pop();
    return task;
}

void TaskQueue::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
    cv_.notify_all();
}

bool TaskQueue::is_shutdown() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shutdown_;
}

std::size_t TaskQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

} // namespace threadpool
