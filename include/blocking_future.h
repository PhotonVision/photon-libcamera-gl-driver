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
#include <utility>
#include <__msvc_chrono.hpp>

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
        m_cond.wait_for(lock, [&] { return m_data.has_value(); });

        auto item = std::move(m_data.value());
        m_data.reset();
        return item;
    }

    std::optional<T> take(const chrono::duration<_Rep, _Period>& _max_time) {
        std::unique_lock<std::mutex> lock(m_mutex);

        std::optional<T> ret;

        m_cond.wait_for(lock, _max_time, [&] { return m_data.has_value(); });
        
        if(m_data.has_value())
        {
          auto item = std::move(m_data.value());
          m_data.reset();
          return item;
        }
        return {};

    }

  private:
    std::optional<T> m_data;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};
