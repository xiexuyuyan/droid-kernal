#ifndef IDL_DROID_OS_I_SERVICE_CALLBACK_H
#define IDL_DROID_OS_I_SERVICE_CALLBACK_H

#include "binder/IInterface.h"
#include "binder/Status.h"

namespace droid {
    namespace os {
    class IServiceCallback : public ::droid::IInterface {
    public:
        virtual ::droid::binder::Status onRegistration(
                const ::std::string& name
                , const ::droid::sp<IBinder>& binder) = 0;
    };
    } // namespace os
} // namespace droid

#endif // IDL_DROID_OS_I_SERVICE_CALLBACK_H
