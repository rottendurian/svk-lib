#ifndef SVKLIB_THREADPOOL_CPP
#define SVKLIB_THREADPOOL_CPP

#include "svk_threadpool.hpp"

#define STP_POOL_IMPLEMENTATION
#include "stp.hpp"

#include "atom/atom.hpp"

namespace svklib {

static atom::pool m_pool;
static std::mutex m_mutex;

threadpool::threadpool() {} 

threadpool* threadpool::get_instance() {
    static std::unique_ptr<threadpool> instance;

    if (instance == nullptr) {

        std::lock_guard<std::mutex> lock(m_mutex);

        if (instance == nullptr) {
            instance.reset(new threadpool());
        }
    }

    return instance.get();
}

void threadpool::add_task(std::function<void()> task) {
    m_pool.add_task(task);
}

void threadpool::add_task(std::function<void()> task,std::atomic_bool* isComplete) {
    isComplete->store(false);
    m_pool.add_task([task,isComplete](){
        task();
        isComplete->store(true);
    });
}

void threadpool::wait_for_tasks() {
    m_pool.wait_for_tasks();
}

void threadpool::empty() {
    m_pool.empty();
}

void threadpool::has_tasks() {
    m_pool.has_tasks();
}

} // namespace svklib

#endif // SVKLIB_THREADPOOL_CPP
