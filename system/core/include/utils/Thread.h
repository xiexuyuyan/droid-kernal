#ifndef DROID_UTILS_THREAD_H
#define DROID_UTILS_THREAD_H

#include "utils/ThreadDefs.h"
#include "utils/RefBase.h"
#include "Errors.h"
#include "utils/Mutex.h"

namespace droid {
    class Thread : virtual public RefBase {
    public:
        explicit Thread(bool canCallJava = true);
        virtual  ~Thread();

        virtual status_t run(
                const char* name
                , int32_t priority = PRIORITY_DEFAULT
                , size_t stack = 0);
        virtual status_t readyToRun();

        pid_t            getTid() const;
        pthread_t        getPthreadId() const;

    protected:
        bool    exitPending() const;

    private:
        virtual bool threadLoop() = 0;

    private:
        static   int         _threadLoop(void* user);
        const    bool        mCanCallJava;
                 thread_id_t mThread;
        mutable  Mutex       mLock;
                 status_t    mStatus;
        volatile bool        mExitPending;
        volatile bool        mRunning;
                 sp<Thread>  mHoldSelf;
                 pid_t       mTid;
    };
}




#endif //DROID_UTILS_THREAD_H
