#ifndef DROID_ISERVICE_MANAGER_H
#define DROID_ISERVICE_MANAGER_H

#include "binder/IInterface.h"
#include "utils/Errors.h"
#include "utils/String16.h"

namespace droid {
    class IServiceManager : public IInterface {
    public:

        static const int DUMP_FLAG_PRIORITY_DEFAULT = 1 << 3;

        virtual status_t addService(const String16& name
                                    , const sp<IBinder>& service
                                    , bool allowIsolated = false
                                    , int dumpSysFlags
                                        = DUMP_FLAG_PRIORITY_DEFAULT) = 0;
    };
    sp<IServiceManager> defaultServiceManager();
} // namespace droid

#endif // DROID_ISERVICE_MANAGER_H