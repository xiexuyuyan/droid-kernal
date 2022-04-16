#include "binder/BpBinder.h"

#include "log/log.h"
#include "binder/IPCThreadState.h"

#undef TAG
#define TAG "BpBinder.cpp"

namespace droid {

    BpBinder *BpBinder::create(int32_t handle) {
        return new BpBinder(handle);
    }

    BpBinder::BpBinder(int32_t handle)
        : mHandle(handle) {
        LOGF_D(TAG, "BpBinder: constructor() handle = %d", handle);
    }

    status_t BpBinder::transact(
            uint32_t code
            , const Parcel &data
            , Parcel *reply
            , uint32_t flags) {

        // todo(20220416-144910 some...)

        status_t status = IPCThreadState::self()->transact(
                mHandle, code, data, reply, flags);

        return status;
    }
} // namespace droid
