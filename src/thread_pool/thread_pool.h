// Based on https://github.com/progschj/ThreadPool.

#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

class TThreadPool {
public:
    TThreadPool(std::size_t threadsCount=std::thread::hardware_concurrency());

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    ~TThreadPool();

private:
    std::vector<std::thread> Threads;
    std::queue<std::function<void()>> Tasks;

    std::mutex Mutex;
    std::condition_variable Condition;
    bool IsDone = false;
};

template<class F, class... Args>
auto TThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(Mutex);

        if (IsDone) {
            throw std::runtime_error("enqueue on stopped TThreadPool");
        }

        Tasks.emplace([task](){ (*task)(); });
    }

    Condition.notify_one();
    return res;
}

