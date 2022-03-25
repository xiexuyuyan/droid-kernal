#ifndef DROID_ISERVICE_MANAGER_H
#define DROID_ISERVICE_MANAGER_H

#include "binder/IInterface.h"

namespace droid {
    class IServiceManager : public IInterface {

    };
    sp<IServiceManager> defaultServiceManager();
} // namespace droid

#endif // DROID_ISERVICE_MANAGER_H