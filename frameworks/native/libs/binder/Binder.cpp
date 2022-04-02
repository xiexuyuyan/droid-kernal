#include "binder/Binder.h"
#include "log/log.h"

#undef TAG
#define TAG "Binder.cpp"

namespace droid {
    IBinder::IBinder() : RefBase() {
        LOG_D(TAG, "IBinder: constructor");
    }
    BBinder::BBinder() {
        LOG_D(TAG, "BBinder: constructor");
    }
} // namespace droid