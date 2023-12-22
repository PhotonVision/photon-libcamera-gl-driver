#pragma once

#include <cstddef>
#include <condition_variable>
#include <mutex>

// A poor substitute for C++20's latch
// This one is blocking on more operations, but
// exposes a similar API

class Latch {
public:
    explicit Latch(ptrdiff_t expected): expected(expected) {};

    ~Latch() = default;
    Latch(const Latch&) = delete;
    Latch& operator=(const Latch&) = delete;

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
    ptrdiff_t expected;
    std::condition_variable cv;
    std::mutex mut;
};