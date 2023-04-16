#ifndef SVKLIB_THREADPOOL_CPP
#define SVKLIB_THREADPOOL_CPP

#include "svklib.hpp"

#define STP_POOL_IMPLEMENTATION
#include "stp.hpp"

namespace svklib {

class threadpoolImpl : public threadpool {
public:
    threadpoolImpl(threadpoolImpl const&) = delete;
    void operator=(threadpoolImpl const&) = delete;

    static threadpoolImpl* get_instance();
    void add_task(std::function<void()> task);
    void add_task(std::function<void()> task,std::atomic_bool* isComplete);
    void wait_for_tasks();
    void empty();
    void has_tasks();

private:
    friend class std::unique_ptr<threadpoolImpl>;
    threadpoolImpl();
    // threadpoolImpl(threadpoolImpl const&); // Don't Implement
    // void operator=(threadpoolImpl const&); // Don't implement
    static std::mutex m_mutex;
    stp::pool m_pool;
};

std::mutex threadpoolImpl::m_mutex;

threadpoolImpl::threadpoolImpl() 
    :m_pool(stp::pool()) {}

threadpoolImpl* threadpoolImpl::get_instance() {
    static std::unique_ptr<threadpoolImpl> instance;

    if (instance == nullptr) {

        std::lock_guard<std::mutex> lock(m_mutex);

        if (instance == nullptr) {
            instance.reset(new threadpoolImpl());
        }
    }

    return instance.get();
}

void threadpoolImpl::add_task(std::function<void()> task) {
    m_pool.add_task(task);
}

void threadpoolImpl::add_task(std::function<void()> task,std::atomic_bool* isComplete) {
    isComplete->store(false);
    m_pool.add_task([task,isComplete](){
        task();
        isComplete->store(true);
    });
}

void threadpoolImpl::wait_for_tasks() {
    m_pool.wait_for_tasks();
}

void threadpoolImpl::empty() {
    m_pool.empty();
}

void threadpoolImpl::has_tasks() {
    m_pool.has_tasks();
}

threadpool* threadpool::get_instance() {
    threadpool* pool = threadpoolImpl::get_instance();
    return pool;
}    

} // namespace svklib

#endif // SVKLIB_THREADPOOL_CPP