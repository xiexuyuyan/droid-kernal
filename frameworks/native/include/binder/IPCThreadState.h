#ifndef DROID_IPC_THREAD_STATE_H
#define DROID_IPC_THREAD_STATE_H

#include "binder/ProcessState.h"

#include "binder/Parcel.h"

namespace droid {
    class IPCThreadState {
    public:
               void             flushCommands();
        static IPCThreadState*  self();
               status_t         transact(int32_t handle
                                         , uint32_t code
                                         , const Parcel& data
                                         , Parcel* reply
                                         , uint32_t flags);

    private:
                                IPCThreadState();
                                ~IPCThreadState();
               void             clearCaller();
        static void             threadDestructor(void* state);
        const  sp<ProcessState> mProcess;
        pid_t                   mCallingPid;
        const char*             mCallingSid;
        uid_t                   mCallingUid;
    };

} // namespace droid


#endif // DROID_IPC_THREAD_STATE_H