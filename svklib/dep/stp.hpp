#ifndef __STP_POOL_HPP 
#define __STP_POOL_HPP

#ifdef _DEBUG
    #include <iostream> // debug std::cout
#endif
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace stp {

class pool {
public:
    pool();
    pool(uint32_t thread_count);
    ~pool();

    void add_task(std::function<void()> task);
    void wait_for_tasks();
    bool has_tasks();
    void empty();

private:
    void spawn_threads(uint32_t count);
    void join_threads();
    void wait_for_task();

    bool _stop_requested = false;
    std::mutex queue_mutex;
    std::condition_variable queue_cond;
    std::queue<std::function<void()>> queue;
    std::vector<std::thread> threads;
};

#ifdef STP_POOL_IMPLEMENTATION

pool::pool() {
    spawn_threads(std::thread::hardware_concurrency());
}

pool::pool(uint32_t thread_count) {
    spawn_threads(thread_count);
}

pool::~pool() {
    join_threads();
}

void pool::spawn_threads(uint32_t count) {
    _stop_requested = false;
    size_t original_size = threads.size();
    threads.resize(original_size+count);
#ifdef _DEBUG
    std::cout << "Thread count: " << threads.size() << std::endl;
#endif
    for (size_t i = original_size; i < threads.size(); i++) {
        threads[i] = std::thread(&pool::wait_for_task, this);
    }
}

void pool::join_threads() {
    while (has_tasks() && _stop_requested == false) {
        std::this_thread::yield();
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        _stop_requested = true;
        // queue.push(nullptr);
        queue_cond.notify_all();
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

void pool::add_task(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    queue.push(task);
    queue_cond.notify_one();
}

void pool::wait_for_tasks() {
    while (has_tasks()) {
        std::this_thread::yield();
    }
}

bool pool::has_tasks() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return !queue.empty();
}

void pool::empty() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    while (!queue.empty()) {
        queue.pop();
    }
}

void pool::wait_for_task() {
    while (true) {
        std::function<void()> task = nullptr;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cond.wait(lock, [this] { return _stop_requested || !queue.empty(); });
            if (_stop_requested) {
                return;
            }
            task = queue.front();
            queue.pop();
        }
        if (task) {
            task();
        }
    }
}

#endif // STP_POOL_IMPLEMENTATION

} // namespace stp

#endif // __STP_POOL_HPP
