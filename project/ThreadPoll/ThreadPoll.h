#pragma once 
#include <map>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
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
    // 添加任务
    void push(std::function<void(void)> task);
    
    template<class T, class... Args> 
    std::future<class std::result_of<T(Args...)>::type> pushTask(T && t, Args && ... args) {
        // 模板函数的声明和实现体要写在同一个头文件中  
        /*
        管理互斥锁类
        会自动对互斥锁加锁和解锁
        当locker对象被析构时会自动解锁
        */ 
        // 添加作用域限制locker，让locker提前析构，以解锁，让cond去唤醒消费者线程工作
        {
            std::lock_guard<std::mutex> locker(_taskMutex);
            _taskQueue.emplace(task);
        }
        /*
            使用emplace在()里面构造对象时效率高，已经构造出来了再传递和push效率一样
            因为此时emplace也是调用的拷贝构造
        */
        _cond.notify_one();    
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