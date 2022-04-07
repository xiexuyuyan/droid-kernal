#ifndef DROID_SERVICEMANAGER_H
#define DROID_SERVICEMANAGER_H

#include <map>
#include <vector>
#include "droid/os/BnServiceManager.h"
#include "droid/os/IServiceCallback.h"

namespace droid {
    using os::IServiceCallback;

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

    private:
        struct Service {
            sp<IBinder> binder;
            bool allowIsolated;
            int32_t dumpPriority;
            bool hasClients = false;
            bool guaranteeClient = false;
            pid_t debugPid = 0;

            // todo(20220403-145951 impl this ref counter)
            ssize_t getNodeStrongRefCount();
        };

        using ServiceMap = std::map<std::string, Service>;
        using ServiceCallbackMap = std::map<std::string, std::vector<sp<IServiceCallback>>>;

        ServiceMap mNameToService;
        ServiceCallbackMap mNameTooRegistrationCallback;
    };
} // namespace droid

#endif //DROID_SERVICEMANAGER_H
