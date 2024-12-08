#pragma once 
#include <map>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>

class ThreadPool {
public:
    // 默认最大线程数量为电脑线程可行数，通过API函数获取
    ThreadPool(int min = 1, int max = std::thread::hardware_concurrency());
    ~ThreadPool();
    // 添加任务
    void push(std::function<void(void)> task);

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