#include "droid/os/IServiceManager.h"
#include "droid/os/BnServiceManager.h"

#include "log/log.h"

#undef TAG
#define TAG "IServiceManager.cpp"

namespace droid {
    namespace os {
        BnServiceManager::BnServiceManager() {
            LOG_D(TAG, "BnServiceManager: constructor");
            // todo(20220402-140212 )
        }
    } // namespace os
} // namespace droid