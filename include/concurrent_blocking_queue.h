/*
 * Copyright (C) Photon Vision.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

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
