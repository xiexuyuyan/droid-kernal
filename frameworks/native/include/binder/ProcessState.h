#ifndef DROID_PROCESS_STATE_H
#define DROID_PROCESS_STATE_H

#include "binder/IBinder.h"
#include "utils/KeyedVector.h"
#include "utils/Mutex.h"

#include "utils/String8.h"

namespace droid {

    class ProcessState: public virtual RefBase {
    public:
        static sp<ProcessState> self();
        sp<IBinder> getContextObject(const sp<IBinder>& caller);
        sp<IBinder> getStrongProxyForHandle(int32_t handle);

    private:
        explicit ProcessState(const char* driver);
        ~ProcessState() override;

        ProcessState& operator=(const ProcessState& o);

        struct handle_entry {
            IBinder* binder;
            RefBase::weakref_type* refs;
        };

        handle_entry* lookupHandleLocked(int32_t handle);

        String8 mDriverName;
        int mDriverFD;
        void* mVMStart;

        mutable Mutex mLock;

        Vector<handle_entry> mHandleToObject;
    };

} // namespace droid

#endif // DROID_PROCESS_STATE_H