#include "droid/os/IServiceManager.h"
#include "droid/os/BpServiceManager.h"

#include "log/log.h"

#undef TAG
#define TAG "IServiceManager.cpp"

namespace droid::os {
    IMPLEMENT_META_INTERFACE(ServiceManager, "droid.os.IServiceManager")
}

#include "droid/os/BpServiceManager.h"
namespace droid::os {

    BpServiceManager::BpServiceManager(
            const ::droid::sp<::droid::IBinder> &impl)
        : BpInterface<IServiceManager>(impl) {
        LOGF_ASSERT(impl != nullptr, "BpServiceManager: constructor");
        LOG_D(TAG, "BpServiceManager: constructor");
    }


    ::droid::binder::Status BpServiceManager::addService(
            const std::string& name
            , const sp<::droid::IBinder>& service
            , bool allowIsolated
            , int dumpsysPriority) {
        ::droid::Parcel data;
        ::droid::Parcel reply;

        // todo(20220416-150758 complete addService...)
        LOGF_D(TAG, "addService: name = %s", name.c_str());

        remote()->transact(0, data, &reply);

        return ::droid::binder::Status::fromExceptionCode(10086);
    }
}


#include "droid/os/BnServiceManager.h"
namespace droid::os {
    BnServiceManager::BnServiceManager() {
        LOG_D(TAG, "BnServiceManager: constructor");
        // todo(20220402-140212 )
    }
} // namespace droid