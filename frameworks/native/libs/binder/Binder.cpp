#include "binder/Binder.h"
#include "log/log.h"
#include "binder/IBinder.h"
#include "binder/BpBinder.h"
#include "binder/IInterface.h"


#undef TAG
#define TAG "Binder.cpp"

namespace droid {
    IBinder::IBinder() : RefBase() {
        LOG_D(TAG, "IBinder: constructor");
    }

    sp<IInterface> IBinder::queryLocalInterface(
            const String16& _d/*descriptor*/) {
        char d[_d.size()];
        utf16_to_utf8(_d.string(), _d.size()
                      , d, _d.size());
        LOGF_D(TAG, "queryLocalInterface: descriptor = %s"
               , d);
        return nullptr;
    }


    BBinder* IBinder::localBinder() {
        return nullptr;
    }

    BpBinder* IBinder::remoteBinder() {
        LOG_D(TAG, "remoteBinder: ");
        return nullptr;
    }

    BBinder::BBinder() {
        LOG_D(TAG, "BBinder: constructor");
    }

    status_t BBinder::transact(
            uint32_t code
            , const Parcel &data
            , Parcel *reply
            , uint32_t flags) {
        // todo(20220416-161905 no lint next line)
        return NO_ERROR;
    }

    BpRefBase::BpRefBase(const sp<IBinder>& o)
        : mRemote(o.get()), mRefs(nullptr), mState(0){
        extendObjectLifetime(OBJECT_LIFETIME_WEAK);
        LOGF_ASSERT(mRemote != nullptr, "BpRefBase: constructor");
        if (mRemote) {
            mRemote->incStrong(this);
            mRefs = mRemote->creatWeak(this);
        }
    }
} // namespace droid