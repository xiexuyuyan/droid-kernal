#include "ServiceManager.h"

#include "log/log.h"

using ::droid::binder::Status;

#undef TAG
#define TAG "ServiceManager.cpp"

namespace droid {
    ServiceManager::ServiceManager() {
        LOG_D(TAG, "ServiceManager: constructor");
    }

    binder::Status ServiceManager::addService(
            const std::string &name
            , const sp<IBinder> &binder
            , bool allowIsolated
            , int32_t dumpPriority) {
        LOG_D(TAG, "addService: ");
        return Status::ok();
    }
} // namespace droid
