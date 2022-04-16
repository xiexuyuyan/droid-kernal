#ifndef DROID_BP_BINDER_H
#define DROID_BP_BINDER_H

#include "binder/IBinder.h"

namespace droid {
    class BpBinder : public IBinder {
    public:
        static BpBinder* create(int32_t handle);
        virtual status_t transact(
                            uint32_t code
                            , const Parcel& data
                            , Parcel* reply
                            , uint32_t flags = 0) final;

    protected:
                         BpBinder(int32_t handle);
    private:
        const  int32_t   mHandle;
    };
} // namespace droid

#endif
