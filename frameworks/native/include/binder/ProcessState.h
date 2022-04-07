#ifndef DROID_PROCESS_STATE_H
#define DROID_PROCESS_STATE_H

#include "binder/IBinder.h"
#include "utils/KeyedVector.h"
#include "utils/Mutex.h"

#include "utils/String8.h"

namespace droid {

    class ProcessState: public virtual RefBase {
    public:
        static  sp<ProcessState>     self();
        static  sp<ProcessState>     initWithDriver(const char* driver);
                sp<IBinder>          getContextObject(const sp<IBinder>& caller);
        typedef bool                 (*context_check_func)(
                                                    const String8& name
                                                    // todo(20220407-143308 String16)
                                                    , const sp<IBinder>& caller
                                                    , void* userData);
                bool                 becomeContextManager(
                                                    context_check_func checkFunc
                                                    , void* userData);
                sp<IBinder>          getStrongProxyForHandle(int32_t handle);
                status_t             setThreadPoolMaxThreadCount(size_t maxThreads);
                String8              getDriverName();
                enum class CallRestriction {
                    NONE,
                    ERROR_IF_NOT_ONEWAY,
                    FATAL_IF_NOT_ONEWAY
                };
                void                setCallRestriction(CallRestriction restriction);

    private:
        friend class IPCThreadState;
        explicit                     ProcessState(const char* driver);
                                     ~ProcessState() override;

                ProcessState&        operator=(const ProcessState& o);

                struct handle_entry {
                    IBinder* binder;
                    RefBase::weakref_type* refs;
                };

                handle_entry*        lookupHandleLocked(int32_t handle);

                String8              mDriverName;
                int                  mDriverFD;
                void*                mVMStart;
                size_t               mMaxThreads;

        mutable Mutex                mLock;

                Vector<handle_entry> mHandleToObject;
                CallRestriction      mCallRestriction;
    };

} // namespace droid

#endif // DROID_PROCESS_STATE_H