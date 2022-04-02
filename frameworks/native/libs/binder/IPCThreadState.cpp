#include "binder/IPCThreadState.h"

#include <atomic>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/android/binder.h>

#include "log/log.h"

#undef TAG
#define TAG "IPCThreadState.cpp"

namespace droid {
    static pthread_mutex_t gTLSMutex = PTHREAD_MUTEX_INITIALIZER;
    static std::atomic<bool> gHaveTLS(false);
    static pthread_key_t gTLS = 0;
    static std::atomic<bool> gShutdown(false);
    // todo(20220330-111301 why android source use implicit constructor = bool)


    IPCThreadState *droid::IPCThreadState::self() {
        if (gHaveTLS.load(std::memory_order_acquire)) {
restart:
            const pthread_key_t key = gTLS;
            IPCThreadState* state =
                    (IPCThreadState*) pthread_getspecific(key);
            if (state)
                return state;
            return new IPCThreadState;
        }

        if (gShutdown.load(std::memory_order_relaxed)) {
            LOG_W(TAG, "self: Calling during shutdown");
            return nullptr;
        }

        pthread_mutex_lock(&gTLSMutex);
        if (!gHaveTLS.load(std::memory_order_relaxed)) {
            int key_create_value = pthread_key_create(
                    &gTLS, threadDestructor);
            if (key_create_value != 0) {
                pthread_mutex_unlock(&gTLSMutex);
                LOGF_E(TAG, "self: Unable to create TLS key"
                            ", expect a crash %d", key_create_value);
                return nullptr;
            }
            gHaveTLS.store(true, std::memory_order_release);
        }
        pthread_mutex_unlock(&gTLSMutex);
        goto restart;
    }

    IPCThreadState *IPCThreadState::selfOrNull() {
        if (gHaveTLS.load(std::memory_order_acquire)) {
            const pthread_key_t key = gTLS;
            IPCThreadState* state =
                    (IPCThreadState*) pthread_getspecific(key);
            return state;
        }
        return nullptr;
    }


    void IPCThreadState::clearCaller() {
        mCallingPid = getpid();
        mCallingSid = nullptr;
        mCallingUid = getuid();
    }

    void IPCThreadState::flushCommands() {
        // todo(20220330-145853 to impl this, it called by @threadDestructor())
    }

    status_t IPCThreadState::transact(
            int32_t handle, uint32_t code
            , const Parcel &data, Parcel *reply
            , uint32_t flags) {
        // todo(20220330-145755 to complete IPCThread's transact)
        LOG_D(TAG, "transact: todo");
        return 0;
    }

    IPCThreadState::IPCThreadState()
        : mProcess(ProcessState::self()) {
        pthread_setspecific(gTLS, this);
        clearCaller();
        // todo(20220330-145821 to complete IPCThreadState's constructor)
        LOGF_D(TAG
               , "IPCThreadState: constructor pid = %d, uid = %d"
               , mCallingPid, mCallingUid);
    }

    IPCThreadState::~IPCThreadState() {
        LOG_D(TAG, "~IPCThreadState: destructor");
    }

    void IPCThreadState::threadDestructor(void *state) {
        LOG_D(TAG, "threadDestructor: ");
        // todo(20220330-162320 why main process exit, it doesn't reach here)
        IPCThreadState* const self = static_cast<IPCThreadState*>(state);
        if (self) {
            self->flushCommands();
            if (self->mProcess->mDriverFD >= 0) {
                ioctl(self->mProcess->mDriverFD
                      , BINDER_THREAD_EXIT, 0);
                // todo(20220330-110428 impl ioctl BINDER_THREAD_EXIT in driver)
            }
            delete self;
        }
    }
}