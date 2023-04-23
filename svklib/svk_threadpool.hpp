#ifndef SVKLIB_THREADPOOL_HPP
#define SVKLIB_THREADPOOL_HPP

namespace svklib {

class threadpool {
public:
    threadpool(threadpool const&) = delete;
    void operator=(threadpool const&) = delete;

    static threadpool* get_instance();
    void add_task(std::function<void()> task);
    void add_task(std::function<void()> task,std::atomic_bool* isComplete);
    void wait_for_tasks();
    void empty();
    void has_tasks();

private:
    threadpool();

    // threadpool(threadpool const&);     // Don't Implement
    // void operator=(threadpool const&); // Don't implement
};

} // namespace svklib

#endif // SVKLIB_THREADPOOL_HPP
