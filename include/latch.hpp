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
#include <cstddef>
#include <mutex>

// A poor substitute for C++20's latch
// This one is blocking on more operations, but
// exposes a similar API

class Latch {
  public:
    explicit Latch(std::ptrdiff_t expected) : expected(expected) {}

    ~Latch() = default;
    Latch(const Latch &) = delete;
    Latch &operator=(const Latch &) = delete;

    void count_down() {
        std::unique_lock lock(mut);
        expected -= 1;
        if (expected == 0) {
            lock.unlock();
            cv.notify_all();
        }
    }

    void wait() {
        std::unique_lock lock(mut);
        cv.wait(lock, [this] { return expected == 0; });
    }

    void arrive_and_wait() {
        std::unique_lock lock(mut);
        expected -= 1;
        if (expected == 0) {
            lock.unlock();
            cv.notify_all();
        } else {
            cv.wait(lock, [this] { return expected == 0; });
        }
    }

  private:
    std::ptrdiff_t expected;
    std::condition_variable cv;
    std::mutex mut;
};
