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

void ThreadPool::push(std::function<void(void)> task) {
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

void ThreadPool::manager(void) {
    while (!_stop.load()) {
        // 搁三秒判断一次
        std::this_thread::sleep_for(std::chrono::seconds(3));
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
                    return;
                } 
            }
            task = move(_taskQueue.front());  // 转成右值，移动构造
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
        if (it.second.joinable()) {
            it.second.join();
        }
    }
    if (_managr->joinable()) {
        _managr->join();
    }
    delete _managr;
}

void add(int x, int y) {
    std::cout << x + y << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

int main() {
    ThreadPool pool;

    for (int i = 0; i < 10; i ++) {
        auto obj = std::bind(add, i, i + 1);
        pool.push(obj);
    }
    
    getchar();

    return 0;
}

