#ifndef DROID_BINDER_H
#define DROID_BINDER_H

#include "binder/IBinder.h"

namespace droid {
    class BBinder : public IBinder {
    public:
        BBinder();
    };
}

#endif // DROID_BINDER_H