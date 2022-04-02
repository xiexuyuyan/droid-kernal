#ifndef DROID_SERVICEMANAGER_H
#define DROID_SERVICEMANAGER_H

#include "droid/os/BnServiceManager.h"

namespace droid {
    class ServiceManager
            : public os::BnServiceManager
            , public IBinder::DeathRecipient{
    public:
        ServiceManager();

        binder::Status addService(
                const std::string& name
                , const sp<IBinder>& binder
                , bool allowIsolated
                , int32_t dumpPriority) override;
    };
} // namespace droid

#endif //DROID_SERVICEMANAGER_H
