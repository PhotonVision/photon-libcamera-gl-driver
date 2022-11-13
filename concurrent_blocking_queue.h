#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T> class ConcurrentBlockingQueue {
  public:
    ConcurrentBlockingQueue() = default;

    T pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [&] { return !m_queue.empty(); });

        auto item = std::move(m_queue.front());
        m_queue.pop();
        return item;
    }

    std::optional<T> try_pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return std::nullopt;
        }

        auto item = std::move(m_queue.front());
        m_queue.pop();
        return item;
    }

    void push(const T &item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(item);
        lock.unlock();
        m_cond.notify_one();
    }

    void push(T &&item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(std::forward<T>(item));
        lock.unlock();
        m_cond.notify_one();
    }

    template <typename... Args> void emplace(Args &&...args) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace(std::forward<Args>(args)...);
        lock.unlock();
        m_cond.notify_one();
    }

  private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};
