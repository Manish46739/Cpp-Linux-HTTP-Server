#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace threadpool {

// Move-only task interface (std::function requires copyable callables).
class ITask {
public:
    virtual ~ITask() = default;
    virtual void run() = 0;
};

template <typename F>
class LambdaTask : public ITask {
public:
    explicit LambdaTask(F&& f) : f_(std::forward<F>(f)) {}

    void run() override { f_(); }

private:
    std::decay_t<F> f_;
};

template <typename F>
std::unique_ptr<ITask> make_task(F&& f) {
    return std::make_unique<LambdaTask<F>>(std::forward<F>(f));
}

} // namespace threadpool
