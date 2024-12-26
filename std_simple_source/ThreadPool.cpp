#include <mutex>
#include <vector>
#include <thread>
#include <queue>
#include <iostream>
#include <functional>
#include <condition_variable>

class ThreadPool {
public:
    explicit ThreadPool(int capacity) : capacity(capacity), stop(false) {
        for (int i = 0; i < capacity; i ++) {
            workers.emplace_back(std::thread(&ThreadPool::worker, this));
        }
    }
    ThreadPool(const ThreadPool&) noexcept = delete;
    ThreadPool& operator=(const ThreadPool&) noexcept = delete;

    void push(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> locker(mtx);
            taskQueue.push(task);
        }
        cond.notify_one();
    }

    void worker() {
        while (!stop) {
            std::function<void()> task = nullptr;
            {
                std::unique_lock<std::mutex> locker(mtx);
                cond.wait(locker, [&]() {
                    return !taskQueue.empty() || stop;
                });
                if (!taskQueue.empty()) {
                    task = std::move(taskQueue.front());
                    taskQueue.pop();
                }
            }
            if (task != nullptr) {
                task();
            }
        }
    }

    ~ThreadPool() {
        stop = true;
        cond.notify_all();
        for (auto &it : workers) {
            it.join();
        }
    }
private:
    bool stop;
    int capacity;
    std::vector<std::thread> workers;
    std::mutex mtx;
    std::condition_variable cond;
    std::queue<std::function<void()>> taskQueue;
};

void sum(int x, int y) {
    std::cout << x + y << "\n";
}

int main() {
    ThreadPool pool(3);
    for (int i = 1; i < 10; i ++) {
        std::function<void()> f = std::bind(sum, i, i);
        pool.push(f);
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
