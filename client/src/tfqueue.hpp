#pragma once

#include <queue>
#include <mutex>

template <typename T>
class TFQueue {
private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable cv;
public:
    std::optional<T> dequeue() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return !queue.empty(); });

        if (queue.empty()) return {}; // Just in case of spurious wakeup
        T value = std::move(queue.front());
        queue.pop();
        return value;
    }

    void enqueue(T&& value) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(std::move(value));
        }
        cv.notify_one();
    }

    class Iterator {
    private:
        std::queue<T>* queue;
        std::unique_lock<std::mutex> lock;

    public:
        Iterator(std::queue<T>* q, std::unique_lock<std::mutex>&& l)
            : queue(q), lock(std::move(l)) {}

        Iterator() : queue(nullptr) {}

        T& operator*() {
            return queue->front();
        }

        Iterator& operator++() {
            queue->pop();
            if (queue->empty()) {
                queue = nullptr;
            }
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return queue != other.queue;
        }
    };

    Iterator begin() {
        std::unique_lock<std::mutex> lock(mutex);
        if (queue.empty()) {
            return end();
        }
        return Iterator(&queue, std::move(lock));
    }

    Iterator end() {
        return Iterator();
    }
};