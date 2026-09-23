#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <cstdint>

class Timer
{
public:
    using Nanos = std::chrono::nanoseconds;
    using Micros = std::chrono::microseconds;
    using Millis = std::chrono::milliseconds;
    using Seconds = std::chrono::seconds;
    using Minutes = std::chrono::minutes;
    using Hours = std::chrono::hours;

    Timer()
        : startTime(std::chrono::steady_clock::now())
    {
    }

    void restart()
    {
        startTime = std::chrono::steady_clock::now();
    }

    template <typename T>
    uint64_t click()
    {
        auto t1 = startTime;
        auto t2 = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<T>(t2 - t1).count();
        startTime = t2;
        return static_cast<uint64_t>(elapsed);
    }

    template <typename T>
    uint64_t glance() const
    {
        auto t1 = startTime;
        auto t2 = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<T>(t2 - t1).count();
        return static_cast<uint64_t>(elapsed);
    }

private:
    std::chrono::steady_clock::time_point startTime;
};

#endif  // TIMER_H