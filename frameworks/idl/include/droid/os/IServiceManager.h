#ifndef IDL_DROID_OS_I_SERVICEMANAGER_H
#define IDL_DROID_OS_I_SERVICEMANAGER_H

#include "binder/IInterface.h"
#include "binder/Status.h"

namespace droid {
    namespace os {
        class IServiceManager : public ::droid::IInterface {
        public:
            enum : int32_t {
                DUMP_FLAG_PRIORITY_DEFAULT = 8,
            };
            virtual ::droid::binder::Status addService(
                    const ::std::string& name
                    , const ::droid::sp<::droid::IBinder>& service
                    , bool allowIsolated
                    , int32_t dumpPriority) = 0;
            // todo(20220402-104426 what is allow isolated)
        };
    } // namespace os
} // namespace droid


#endif
