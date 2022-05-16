#ifndef DROID_IPC_THREAD_STATE_H
#define DROID_IPC_THREAD_STATE_H

#include "binder/ProcessState.h"

#include "binder/Parcel.h"

namespace droid {
    class IPCThreadState {
    public:
        static IPCThreadState*  self();
        static IPCThreadState*  selfOrNull();
               void             flushCommands();
               status_t         transact(int32_t handle
                                         , uint32_t code
                                         , const Parcel& data
                                         , Parcel* reply
                                         , uint32_t flags);
               void             setTheContextObject(sp<BBinder> obj);

    private:
                                IPCThreadState();
                                ~IPCThreadState();
               status_t         waitForResponse(
                                    Parcel* reply
                                    , status_t* acquireResult = nullptr);
               status_t         talkWithDriver(bool doReceive = true);
               status_t         writeTransactionData(
                                    int32_t cmd
                                    , uint32_t binderFlags
                                    , int32_t handle
                                    , uint32_t code
                                    , const Parcel& data
                                    , status_t* statusBuffer);
               void             processPostWriteDerefs();
               void             clearCaller();
        static void             threadDestructor(void* state);
        const  sp<ProcessState> mProcess;
               Vector<RefBase::weakref_type*> mPostWriteWeakDerefs;
               Vector<RefBase*> mPostWriteStrongDerefs;
               Parcel           mIn;
               Parcel           mOut;
               status_t         mLastError;
               pid_t            mCallingPid;
               const char*      mCallingSid;
               uid_t            mCallingUid;
    };

} // namespace droid


#endif // DROID_IPC_THREAD_STATE_H