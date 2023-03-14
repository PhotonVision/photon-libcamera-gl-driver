#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>

template <typename T> class BlockingFuture {
  public:
    BlockingFuture() = default;

    void set(T &&item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_data = std::make_optional<>(std::forward<T>(item));
        lock.unlock();
        m_cond.notify_one();
    }

    T take() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [&] { return m_data.has_value(); });

        auto item = std::move(m_data.value());
        m_data.reset();
        return item;
    }

  private:
    std::optional<T> m_data;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};