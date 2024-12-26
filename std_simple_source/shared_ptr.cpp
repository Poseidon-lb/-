#include <iostream>
#include <atomic>

template<class T>
struct ControlBlock {
    T *ptr;
    std::atomic<int> ref_count;

    ControlBlock(T *p) : ptr(p), ref_count(1) {}
    ~ControlBlock() {
        delete ptr;  // 销毁对象
    }
};

template<class T>
class Shared_ptr {
public:
    explicit Shared_ptr(T *ptr = nullptr) {
        if (ptr) {
            controlBlock = new ControlBlock<T>(ptr);
        } else {
            controlBlock = nullptr;
        }
    }

    Shared_ptr(const Shared_ptr &other) {
        controlBlock = other.controlBlock;
        if (controlBlock) {
            controlBlock->ref_count++;  // 增加引用计数
        }
    }

    Shared_ptr& operator=(const Shared_ptr &other) {
        if (this != &other) {
            if (controlBlock && --controlBlock->ref_count == 0) {
                delete controlBlock;
            }
            controlBlock = other.controlBlock;
            if (controlBlock) {
                controlBlock->ref_count++;  // 增加引用计数
            }
        }
        return *this;
    }

    ~Shared_ptr() {
        if (controlBlock && --controlBlock->ref_count == 0) {
            delete controlBlock;  // 引用计数为 0 时销毁对象
        }
    }

    void reset() {
        if (controlBlock && --controlBlock->ref_count == 0) {
            delete controlBlock;
        }
        controlBlock = nullptr;
    }

    T* operator->() const {
        return controlBlock ? controlBlock->ptr : nullptr;
    }

    T& operator*() const {
        return *controlBlock->ptr;
    }

    int use_count() const {
        return controlBlock ? controlBlock->ref_count.load() : 0;
    }

private:
    ControlBlock<T> *controlBlock;
};

class A {
public:
    A() : x(100) {}
    void print() {
        std::cout << x << "\n";
    }
    int x;
};

int main() {
    Shared_ptr<A> ptr(new A());

    ptr->print();  // 调用 print 方法

    return 0;
}