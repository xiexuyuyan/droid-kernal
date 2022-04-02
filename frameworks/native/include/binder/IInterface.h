#ifndef DROID_IINTERFACE_H
#define DROID_IINTERFACE_H

#include "binder/Binder.h"

namespace droid {
    // todo(20220324-123218 what means virtual for inheritance?)
    class IInterface : public virtual RefBase {

    };

    template<typename INTERFACE>
    inline sp<INTERFACE> interface_cast(const sp<IBinder>& obj) {
        return INTERFACE::asInterface(obj);
    }

    template<typename INTERFACE>
    class BnInterface : public INTERFACE, public BBinder {

    };
} // namespace droid

#endif // DROID_IINTERFACE_H