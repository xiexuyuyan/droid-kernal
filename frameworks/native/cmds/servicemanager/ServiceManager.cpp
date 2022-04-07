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

        // todo(20220402-155416 authentication)

        if (binder == nullptr) {
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        }

        // todo(20220403-145535 when does @{binder->remoteBinder} not null?)
        if (binder->remoteBinder() != nullptr) {
            LOG_D(TAG, "addService: here, @binder->remoteBinder != null");
            // todo(20220403-145122 linkToDeath)
            // && binder->linkToDeath(this) != OK
            LOGF_E(TAG, "addService: "
                        "Could not linkToDeath when adding %s"
                        , name.c_str());
        }

        mNameToService[name] = Service {
            .binder = binder,
            .allowIsolated = allowIsolated,
            .dumpPriority = dumpPriority,
            // todo(20220403-150418 getDebugPid from context Access.h)
            .debugPid = 0,
        };

        auto it = mNameTooRegistrationCallback.find(name);
        if (it != mNameTooRegistrationCallback.end()) {
            // todo(20220403-162259 @onRegistration callback)
            LOG_D(TAG, "addService: !=");
        } else {
            LOG_D(TAG, "addService: ==");
        }


        return Status::ok();
    }

    ssize_t ServiceManager::Service::getNodeStrongRefCount() {
        return 0;
    }
} // namespace droid
