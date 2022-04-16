#include <mutex>
#include "binder/Status.h"
#include "binder/IInterface.h"
#include "binder/IServiceManager.h"
#include "droid/os/IServiceManager.h"
#include "binder/IPCThreadState.h"

#undef TAG
#define TAG "IServiceManager.cpp"

namespace droid {
    using IdlServiceManager = droid::os::IServiceManager;
    using droid::binder::Status;

    // todo(20220416-114333 what means Shim?)
    class ServiceManagerShim : public IServiceManager {
    public:
        explicit ServiceManagerShim(const sp<IdlServiceManager>& impl);
        status_t addService(const String16& name
                            , const sp<IBinder>& service
                            , bool allowIsolated
                            , int dumpsysPriority) override;
    private:
        sp<IdlServiceManager> mTheRealServiceManager;
    };

    ServiceManagerShim::ServiceManagerShim(
            const sp<IdlServiceManager> &impl)
            : mTheRealServiceManager(impl){
        LOGF_D(TAG, "ServiceManagerShim(): constructor");
    }

    status_t ServiceManagerShim::addService(
            const String16 &name
            , const sp<IBinder> &service
            , bool allowIsolated
            ,int dumpsysPriority) {
        Status status = mTheRealServiceManager->addService(
                String8(name).c_str()
                , service
                , allowIsolated
                , dumpsysPriority);
        return status.exceptionCode();
    }


    [[clang::no_destroy]]
        static std::once_flag gSmOnce;
    [[clang::no_destroy]]
        static sp<IServiceManager> gDefaultServiceManager;
    // todo(20220324-125707 what means no_destroy? once_flag? call_once?)
    sp<IServiceManager> defaultServiceManager() {
        LOG_D(TAG, "defaultServiceManager: ");
        std::call_once(gSmOnce, []() {
            sp<IdlServiceManager> impl = nullptr;
            // todo(20220416-114012 while (sm == nullptr))
            sp<IBinder> proxy =
                    ProcessState::self()->getContextObject(nullptr);
            // todo(20220326-130700 test get then test cast)
            impl = interface_cast<IdlServiceManager>(proxy);
            gDefaultServiceManager = new ServiceManagerShim(impl);
        });

        return gDefaultServiceManager;
    }
} // namespace droid