#ifndef IDL_DROID_OS_BP_SERVICEMANAGER_H
#define IDL_DROID_OS_BP_SERVICEMANAGER_H

#include "binder/IBinder.h"
#include "binder/IInterface.h"
#include "droid/os/IServiceManager.h"

namespace droid::os {
    class BpServiceManager : public ::droid::BpInterface<IServiceManager> {
    public:
        explicit BpServiceManager(const ::droid::sp<::droid::IBinder>& impl);
        ::droid::binder::Status addService(
                const ::std::string& name
                , const ::droid::sp<::droid::IBinder>& service
                , bool allowIsolated
                , int dumpsysPriority) override;
    };
}


#endif // IDL_DROID_OS_BP_SERVICEMANAGER_H
