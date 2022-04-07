#include "binder/Binder.h"
#include "log/log.h"
#include "binder/IBinder.h"
#include "binder/BpBinder.h"


#undef TAG
#define TAG "Binder.cpp"

namespace droid {
    IBinder::IBinder() : RefBase() {
        LOG_D(TAG, "IBinder: constructor");
    }

    BBinder* IBinder::localBinder() {
        return nullptr;
    }

    BpBinder* IBinder::remoteBinder() {
        LOG_D(TAG, "remoteBinder: ");
        return nullptr;
    }

    BBinder::BBinder() {
        LOG_D(TAG, "BBinder: constructor");
    }
} // namespace droid