#pragma once

#include <queue>
#include <mutex>
#include <optional>

template <typename T>
class TFQueue {
private:
    std::queue<std::optional<T>> queue;
    std::mutex mutex;
    std::condition_variable cv;
public:
    TFQueue() = default;
    TFQueue(const TFQueue&) = delete;

    std::optional<T> dequeue() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return !queue.empty(); });
        auto value = std::move(queue.front());
        queue.pop();
        return value;
    }

    void enqueue(T&& value) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(std::make_optional(std::move(value)));
        }
        cv.notify_one();
    }

    void finish() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(std::nullopt);
        }
        cv.notify_one();
    }

    class Iterator {
    private:
        std::queue<std::optional<T>>* queue;
        std::unique_lock<std::mutex> lock;

    public:
        Iterator(std::queue<std::optional<T>>* q, std::unique_lock<std::mutex>&& l)
            : queue(q), lock(std::move(l)) {}

        Iterator() : queue(nullptr) {}

        T& operator*() {
            return queue->front().value();
        }

        Iterator& operator++() {
            queue->pop();
            if (queue->empty() || !queue->front().has_value()) {
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
        if (queue.empty() || !queue.front().has_value()) {
            return end();
        }
        return Iterator(&queue, std::move(lock));
    }

    Iterator end() {
        return Iterator();
    }
};