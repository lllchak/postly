#include "thread_pool.h"

TThreadPool::TThreadPool(std::size_t threadsCount) {
    for (std::size_t i = 0; i < threadsCount; ++i) {
        Threads.emplace_back(
            [this] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(Mutex);
                        Condition.wait(lock, [this]{ return this->IsDone || !this->Tasks.empty(); });
                        if (IsDone && Tasks.empty()) {
                            return;
                        }
                        task = std::move(Tasks.front());
                        Tasks.pop();
                    }
                    task();
                }
            }
        );
    }
}

TThreadPool::~TThreadPool() {
    {
        std::unique_lock<std::mutex> lock(Mutex);
        IsDone = true;
    }

    Condition.notify_all();
    for(auto&& thread : Threads) {
        thread.join();
    }
}
