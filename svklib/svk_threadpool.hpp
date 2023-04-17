#ifndef SVKLIB_THREADPOOL_HPP
#define SVKLIB_THREADPOOL_HPP

// #include "pch.hpp"

namespace svklib {

class threadpool {
private:
    friend class threadpoolImpl;
public:
    threadpool(threadpool const&) = delete;
    void operator=(threadpool const&) = delete;

    static threadpool* get_instance();
    virtual void add_task(std::function<void()> task) = 0;
    virtual void add_task(std::function<void()> task,std::atomic_bool* isComplete) = 0;
    virtual void wait_for_tasks() = 0;
    virtual void empty() = 0;
    virtual void has_tasks() = 0;

private:
    threadpool() {}
    // threadpool(threadpool const&);     // Don't Implement
    // void operator=(threadpool const&); // Don't implement
};

} // namespace svklib

#endif // SVKLIB_THREADPOOL_HPP