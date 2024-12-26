#include <mutex>
#include <vector>
#include <thread>
#include <iostream>
#include <condition_variable>

class TaskQueue {
public:
    TaskQueue(int _size) : size(_size) {}
    void push(const int& task) {
        {
            std::unique_lock<std::mutex> locker(_mutex);
            // while (taskQueue.size() == this->size) {
            //     notFull.wait(locker);
            // }
            notFull.wait(locker, [&] {
                return taskQueue.size() != this->size;
            });
            taskQueue.emplace_back(task);
        }
        notEmpty.notify_one();
    }
    void pop() {
        {
            std::unique_lock<std::mutex> locker(_mutex);
            // while (taskQueue.empty()) {
            //     notEmpty.wait(locker);
            // }
            notEmpty.wait(locker, [&]() {
                return !taskQueue.empty();
            });
            int t = taskQueue.back();
            taskQueue.pop_back();
            std::cout << "pop: " << t << "\n";
        }
        notFull.notify_one();
    }
    bool empty() {
        std::unique_lock<std::mutex> locker(_mutex);
        return taskQueue.empty();
    }
    bool full() {
        std::unique_lock<std::mutex> locker(_mutex);
        return taskQueue.size() == this->size;
    }
private:
    int size;
    std::mutex _mutex;
    std::vector<int> taskQueue;
    std::condition_variable notFull, notEmpty;
};

int main() {
    TaskQueue tq(100);

    constexpr int N = 10;

    std::thread t1[N], t2[N];
    for (int i = 0; i < N; i ++) {
        t1[i] = std::thread(&TaskQueue::push, &tq, i + 1);
        t2[i] = std::thread(&TaskQueue::pop, &tq);
    }

    for (int i = 0; i < N; i ++) {
        t1[i].join();
        t2[i].join();
    }

}
