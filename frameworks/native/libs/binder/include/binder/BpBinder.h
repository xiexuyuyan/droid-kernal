#ifndef DROID_BP_BINDER_H
#define DROID_BP_BINDER_H

#include "binder/IBinder.h"

namespace droid {
    class BpBinder : public IBinder {
    public:
        static BpBinder* create(int32_t handle);
    };
} // namespace droid

#endif
