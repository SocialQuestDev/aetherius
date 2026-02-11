#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class TSQueue {
public:
    void push(T item) {
        std::lock_guard<std::mutex> lock(mux);
        queue.push(std::move(item));
    }

    bool try_pop(T& item) {
        std::lock_guard<std::mutex> lock(mux);
        if (queue.empty()) {
            return false;
        }
        item = std::move(queue.front());
        queue.pop();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mux);
        return queue.empty();
    }

private:
    std::queue<T> queue;
    std::mutex mux;
};
