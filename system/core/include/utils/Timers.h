#ifndef UTILS_TIMERS_H
#define UTILS_TIMERS_H

#include <cstdint>

typedef int64_t nsecs_t; // nano-seconds

enum {
    SYSTEM_TIME_REALTIME  = 0,
    SYSTEM_TIME_MONOTONIC = 1,
};

static constexpr inline nsecs_t milliseconds_to_nanoseconds(
        nsecs_t secs) {
    return secs * 1000000;
}


nsecs_t systemTime(int clock = SYSTEM_TIME_MONOTONIC);

int toMillisecondTimeoutDelay(nsecs_t referenceTime, nsecs_t timeoutTime);

#endif // UTILS_TIMERS_H
