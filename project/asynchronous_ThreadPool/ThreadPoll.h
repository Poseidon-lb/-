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

/*
    F 是可调用对象的类型。 Args是类型包
    F 的类型是 int(int, int)，所以 F(Args...) 表示调用 void(int, int) 类型的函数，传递两个 int 类型的参数
    int(int, int) 函数返回 int，所以 result_of<int(int, int)(int, int)>::type 的结果是 void
*/
    template<class F, class...Args>
    auto pushTask(F && f, Args && ...args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using reType = typename std::result_of<F(Args...)>::type;
        // std::shared_ptr<std::packaged_task<reType()>>
        // 使用共享智能指针，防止pushTask结束后析构后，子线程中的也析构了
        auto task_ptr = std::make_shared<std::packaged_task<reType()>> (
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            /*不知道f会有几个参数 bind消元
            完美转发保留原来类型。f和args可能本来是右值引用，使用就变成了左值，传过去就变左值引用了*/
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