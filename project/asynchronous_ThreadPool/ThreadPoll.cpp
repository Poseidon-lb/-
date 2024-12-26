#include "ThreadPoll.h"
/*
    初始化列列表：直接调用对象的构造函数初始化，省去构造后再赋值的开销
    构造函数内初始化：赋值
*/
ThreadPool::ThreadPool(int min, int max) : _maxThread(max), _minThread(min),
_stop(false), _idleThread(min), _curThread(min) {
    // 参数：线程工作函数的地址，函数的所有者(该线程池的实例对象)
    // 如果不是类成员函数直接指定即可，不需要this
    _managr = new std::thread(&ThreadPool::manager, this);
    for (int i = 0; i < min; i ++) {
        /*  
            push_back: 将已经构造出来的对象移动或拷贝到容器末尾
            emplace_back: 直接在容器末尾构造
            这里传递的是右值会被右值引用，如果是传递的参数就直接末尾构造
            匿名对象在被弹出容器时销毁
            这里时一个右值引用，延长临时变量声明周期
        */
       std::thread t(&ThreadPool::woker, this);
       // 线程对象不允许拷贝
       _workers.insert({t.get_id(), move(t)});
       // 退出容器才销毁
    }
}

void ThreadPool::manager(void) {
    while (!_stop.load()) {
        // 搁三秒判断一次
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int idel = _idleThread.load();
        int cur = _curThread.load();
        // 空闲线程超过一半，销毁两个线程。销毁的是被阻塞的线程
        if (cur > _minThread && idel * 2 > cur) {
            _exitThread.store(2);
            _cond.notify_all();
            std::unique_lock<std::mutex> locker(_exitIdMutex);
            for (auto id : _exIds) {
                auto it = _workers.find(id);
                if (it != _workers.end()) {
                    // 等待线程结束，回收系统资源
                    it->second.join();
                    _workers.erase(it);
                    // 线程对象从map中移除，对象自动销毁
                }
            }
            _exIds.clear();
        } else if (_curThread < _maxThread && _idleThread == 0) {
            std::thread t(&ThreadPool::woker, this);
            _workers.insert({t.get_id(), move(t)});
            _idleThread++;
            _curThread++;
        }
    }
}

void ThreadPool::woker() {
    while (!_stop.load()) {
        std::function<void(void)> task = nullptr;
        /*
            要先加锁再去使用条件变量阻塞，否则可能会阻塞被唤醒后，任务被其他线程把取走了
            此时就不会判断任务队列是否为空
        */
        {
            std::unique_lock<std::mutex> locker(_taskMutex);
            while (_taskQueue.empty()) {
                _cond.wait(locker);       // 条件变量的参数是unique_lock类型
                if (_exitThread.load() > 0) {
                    // 销毁一个任务线程，退出工作函数
                    std::unique_lock<std::mutex> locker(_exitIdMutex);
                    _exIds.emplace_back(std::this_thread::get_id());
                    _curThread--;
                    _exitThread--;
                    _idleThread--;
                    return;
                } 
            }
            task = move(_taskQueue.front());
            _taskQueue.pop();
        }
        _idleThread--;
        task();
        _idleThread++;
    }
}

ThreadPool::~ThreadPool() {
    _stop = true;
    _cond.notify_all();
    for (auto &it : _workers) {
        // 判断对象是否可连接，没有join或者线程分离过
        if (it.second.joinable()) {
            it.second.join();
        }
    }
    if (_managr->joinable()) {
        _managr->join();
    }
    delete _managr;
}

int add(int x, int y) {
    int sum = x + y;
    std::cout << x + y << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return sum;
}

int main() {
    ThreadPool pool;

    for (int i = 0; i < 10; i ++) {
        pool.pushTask(add, i, i + 1);
    }
    
    getchar();

    return 0;
}

