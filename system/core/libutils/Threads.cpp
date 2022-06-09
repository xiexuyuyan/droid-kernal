#include <sys/prctl.h>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include "utils/Thread.h"
#include "utils/DroidThreads.h"
#include "log/log.h"

#undef TAG
#define TAG "Threads.cpp"

typedef void* (*droid_pthread_entry)(void*);

struct thread_data_t {
    thread_func_t entryFUnction;
    void*         userData;
    int           priority;
    char*         threadName;

    static int trampoline(const thread_data_t* t) {
        thread_func_t f = t->entryFUnction;
        void* u = t->userData;
        int prio = t->priority;
        char* name = t->threadName;
        delete t;

        setpriority(PRIO_PROCESS, 0, prio);

        if (name) {
            droidSetThreadName(name);
            free(name);
        }
        return f(u);
    }
};

void droidSetThreadName(const char* name) {
    int hasAt = 0;
    int hasDot = 0;
    const char* s = name;
    while (*s) {
        if (*s == '.') hasDot = 1;
        else if (*s == '@') hasAt = 1;
        s++;
    }
    int len = s - name;
    if (len < 15 || hasAt || !hasDot) {
        s = name;
    } else {
        s = name + len - 15;
    }
    prctl(PR_SET_NAME, (unsigned long)s, 0, 0, 0);
}

int droidCreateRawThreadEtc(
        droid_thread_func_t entryFunction
        , void* userData
        , __attribute__((unused)) const char* threadName
        , int32_t threadPriority
        , size_t threadStackSize
        , droid_thread_id_t* threadId) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (threadPriority != PRIORITY_DEFAULT || threadName != NULL) { // NOLINT
        // though we have a method to find the associated
        // (droid_thread_id_t)pid from pthread_t...
        thread_data_t* t = new thread_data_t;// NOLINT
        t->priority = threadPriority;
        t->threadName = threadName ? strdup(threadName) : NULL;
        t->entryFUnction = entryFunction;
        t->userData = userData;
        entryFunction = (droid_thread_func_t)&thread_data_t::trampoline;
        userData = t;
    }

    if (threadStackSize) {
        pthread_attr_setstacksize(&attr, threadStackSize);
    }

    errno = 0;
    pthread_t thread;
    int result = pthread_create(
            &thread, &attr, (droid_pthread_entry)entryFunction, userData);
    pthread_attr_destroy(&attr);
    if (result != 0) {
        LOGF_E(TAG, "droidCreateRawThreadEtc: "
                   "creat failed (entry=%p, res=%d, %s)\n"
                   "(droid_threadPriority=%d)"
                   , entryFunction, result, strerror(errno)
                   , threadPriority);
        return 0;
    }

    if (threadId != nullptr) {
        *threadId = (droid_thread_id_t)thread;
    }

    return 1;
}

namespace droid {

    Thread::Thread(bool canCallJava)
        : mCanCallJava(canCallJava)
        , mThread(thread_id_t(-1))
        , mLock("Thread::mLock")
        , mStatus(OK)
        , mExitPending(false)
        , mRunning(false)
        , mTid(-1) {
    }

    Thread::~Thread() {

    }

    status_t Thread::run(
            const char* name, int32_t priority, size_t stack) {

        LOGF_ASSERT(name != nullptr
                    , "run: thread name not provided to Thread::run");

        Mutex::Autolock _l(mLock);

        if (mRunning) {
            return INVALID_OPERATION;
        }

        mStatus = OK;
        mExitPending = false;
        mThread = thread_id_t(-1);

        mHoldSelf = this;

        mRunning = true;

        bool res;
        if (mCanCallJava) {

        } else {
            res = droidCreateRawThreadEtc(_threadLoop
                    , this, name, priority, stack, &mThread);
        }

        if (res == false) {
            mStatus = UNKNOWN_ERROR;
            mRunning = false;
            mThread = thread_id_t(-1);
            mHoldSelf.clear();

            return UNKNOWN_ERROR;
        }

        return OK;
    }

    int Thread::_threadLoop(void *user) {
        Thread* const self = static_cast<Thread*>(user);// NOLINT

        sp<Thread> strong(self->mHoldSelf);
        wp<Thread> weak(strong);
        self->mHoldSelf.clear();

        self->mTid = gettid();

        bool first = true;

        do {
            bool result;
            if (first) {
                first = false;
                self->mStatus = self->readyToRun();
                result = (self->mStatus == OK);

                if (result && !self->exitPending()) {
                    result = self->threadLoop();
                }
            } else {
                result = self->threadLoop();
            }

            {
                Mutex::Autolock _l(self->mLock);
                if (result == false || self->mExitPending) {
                    self->mExitPending = true;
                    self->mRunning = false;
                    self->mThread = thread_id_t(-1);
                    // todo(20220609-184338 Condition)
                    // self->mThreadExitedCondition.broadcast();
                    break;
                }
            }
            strong.clear();
            strong = weak.promote();
        } while(true);

        return 0;
    }

    status_t Thread::readyToRun() {
        return OK;
    }

    bool Thread::exitPending() const {
        Mutex::Autolock _l(mLock);
        return mExitPending;
    }

    pid_t Thread::getTid() const {
        Mutex::Autolock _l(mLock);
        pid_t tid;
        if (mRunning) {
            tid = mTid;
        } else {
            LOGF_W(TAG, "getTid: Thread(this=%p): undefined!");
            tid = -1;
        }

        return tid;
    }

    pthread_t Thread::getPthreadId() const {
        Mutex::Autolock _l(mLock);
        pthread_t pthread;
        if (mRunning) {
            pthread = (pthread_t)mThread;
        } else {
            LOGF_W(TAG, "getPthreadId: Thread(this=%p): undefined!");
            pthread = -1;
        }

        return pthread;
    }

}
