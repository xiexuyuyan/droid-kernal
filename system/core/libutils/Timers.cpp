#include <ctime>
#include <climits>
#include "utils/Timers.h"

nsecs_t systemTime(int clock) {
    static const clockid_t clocks[] = {
            CLOCK_REALTIME,
            CLOCK_MONOTONIC,
            CLOCK_PROCESS_CPUTIME_ID,
            CLOCK_THREAD_CPUTIME_ID,
            CLOCK_BOOTTIME
    };
    struct timespec t{};
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[clock], &t);
    return nsecs_t(t.tv_sec) * 1000000000LL + t.tv_nsec;
}

int toMillisecondTimeoutDelay(
        nsecs_t referenceTime, nsecs_t timeoutTime) {
    nsecs_t timeoutDelayMillis;
    if (timeoutTime > referenceTime) {
        /*uint64_t*/auto timeoutDelay =
                uint64_t(timeoutTime - referenceTime);
        // (INT_MAX - 1) * 1000000LL
        // means the biggest number a signed int could save
        if (timeoutDelay > uint64_t((INT_MAX - 1) * 1000000LL)) {
            timeoutDelayMillis = -1;
        } else {
            // make it to millisecond
            timeoutDelayMillis =
                    (nsecs_t)(timeoutDelay + 999999LL) / 1000000LL;
        }
    } else {
        timeoutDelayMillis = 0;
    }

    return (int)timeoutDelayMillis;
}

