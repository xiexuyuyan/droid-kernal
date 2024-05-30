#ifndef IDL_DROID_OS_BN_SERVICEMANAGER_H
#define IDL_DROID_OS_BN_SERVICEMANAGER_H

#include "binder/IInterface.h"
#include "droid/os/IServiceManager.h"

namespace droid {
    namespace os {
        class BnServiceManager :
                public ::droid::BnInterface<IServiceManager> {
        public:
            static constexpr uint32_t TRANSACTION_getService = ::droid::IBinder::FIRST_CALL_TRANSACTION + 0;
            static constexpr uint32_t TRANSACTION_checkService = ::droid::IBinder::FIRST_CALL_TRANSACTION + 1;
            static constexpr uint32_t TRANSACTION_addService = ::droid::IBinder::FIRST_CALL_TRANSACTION + 2;
            explicit BnServiceManager();
        };
    } // namespace os
} // namespace droid

#endif // IDL_DROID_OS_BN_SERVICEMANAGER_H
