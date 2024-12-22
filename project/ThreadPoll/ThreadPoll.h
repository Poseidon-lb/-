#pragma once 
#include <map>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <queue>
#include <mutex>
#include <future>
#include <functional>
#include <condition_variable>

class ThreadPool {
public:
    // 默认最大线程数量为电脑线程可行数，通过API函数获取
    ThreadPool(int min = 2, int max = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<class F, class...Args>
    auto pushTask(F && f, Args && ...args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using reType = typename std::result_of<F(Args...)>::type;
        // std::shared_ptr<std::packaged_task<reType()>>
        auto task_ptr = std::make_shared<std::packaged_task<reType()>> (
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );  
        {
            std::unique_lock<std::mutex> locker(_taskMutex);
            _taskQueue.push([task_ptr]() {
                (*task_ptr)();      
            });
        }
        _cond.notify_one();
        return task_ptr->get_future();
    }

    template<class T> void f(T && t) {}

private:
    void manager(void);
    void woker();
private:
    std::thread *_managr;                               // 管理员
    std::map<std::thread::id, std::thread> _workers; 
    std::vector<std::thread::id> _exIds;                // 已经退出的线程id                  
    std::atomic<int> _minThread, _maxThread;            // 线程数量的上下限
    std::atomic<int> _curThread, _idleThread;           // 总线程和空闲线程数量
    std::atomic<bool> _stop;                
    std::atomic<int> _exitThread;                       // 需要退出几个线程
    std::queue<std::function<void(void)>> _taskQueue;   
    std::mutex _taskMutex;
    std::mutex _exitIdMutex;
    std::condition_variable _cond;
};