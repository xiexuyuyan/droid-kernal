#include "utils/Mutex.h"

#undef TAG
#define TAG "Mutex.h"
#include "log/log.h"

namespace droid {
    int32_t Mutex::lock() {
        mutex.lock();
        LOG_V(TAG, "lock: ");
        return 0;
    }

    void Mutex::unlock() {
        mutex.unlock();
        LOG_V(TAG, "unlock: ");
    }
}