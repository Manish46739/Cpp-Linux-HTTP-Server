#include "threadpool/task_queue.hpp"
#include "threadpool/thread_pool.hpp"

#include <atomic>
#include <gtest/gtest.h>

TEST(TaskQueue, ProcessesEnqueuedTasks) {
    threadpool::TaskQueue queue;
    std::atomic<int> value{0};

    queue.enqueue([&]() { value.store(1); });

    auto task = queue.dequeue();
    ASSERT_TRUE(task);
    (*task)();

    EXPECT_EQ(value.load(), 1);
    queue.shutdown();
}

TEST(TaskQueue, ShutdownUnblocksDequeue) {
    threadpool::TaskQueue queue;
    queue.shutdown();
    EXPECT_TRUE(queue.is_shutdown());
    EXPECT_FALSE(queue.dequeue());
}

TEST(ThreadPool, RunsTasksConcurrently) {
    threadpool::ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
        pool.submit([&]() { counter.fetch_add(1); });
    }

    pool.shutdown();
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPool, MoveOnlyLambdaWorks) {
    threadpool::ThreadPool pool(2);
    std::atomic<bool> ran{false};

    pool.submit([&ran]() { ran.store(true); });
    pool.shutdown();

    EXPECT_TRUE(ran.load());
}
