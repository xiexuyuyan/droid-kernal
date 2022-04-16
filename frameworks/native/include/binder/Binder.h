#ifndef DROID_BINDER_H
#define DROID_BINDER_H

#include "binder/IBinder.h"

namespace droid {
    class BBinder : public IBinder {
    public:
                               BBinder();

        virtual status_t       transact(
                                    uint32_t code
                                    , const Parcel &data
                                    , Parcel *reply
                                    , uint32_t flags) final;
    };

    class BpRefBase : public virtual RefBase {
    protected:
        explicit               BpRefBase(const sp<IBinder>& o);
        inline   IBinder*      remote()       { return mRemote; }
        inline   IBinder*      remote() const { return mRemote; }
    private:
        IBinder* const         mRemote;
        RefBase::weakref_type* mRefs;
        std::atomic<int32_t>   mState;
    };
}

#endif // DROID_BINDER_H