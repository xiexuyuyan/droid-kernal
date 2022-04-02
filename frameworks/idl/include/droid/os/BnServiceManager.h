#ifndef IDL_DROID_OS_BN_SERVICEMANAGER_H
#define IDL_DROID_OS_BN_SERVICEMANAGER_H

#include "binder/IInterface.h"
#include "droid/os/IServiceManager.h"

namespace droid {
    namespace os {
        class BnServiceManager :
                public ::droid::BnInterface<IServiceManager> {
        public:
            explicit BnServiceManager();
        };
    } // namespace os
} // namespace droid

#endif // IDL_DROID_OS_BN_SERVICEMANAGER_H
