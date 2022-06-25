#include "utils/Thread.h"
#include "log/log.h"

#include <pthread.h>
#include <unistd.h>


#undef TAG
#define TAG "DroidThread.cpp"

namespace droid {
    class MThreadState {
    public:
        MThreadState();
        ~MThreadState();

        static MThreadState* self();
        static void* threadDestructor(void* st);
    };
}

namespace droid {
    static pthread_mutex_t gTLSMutex = PTHREAD_MUTEX_INITIALIZER;
    static std::atomic<bool> gHaveTLS(false);
    static pthread_key_t gTLS = 0;

    MThreadState::MThreadState() {
        LOG_D(TAG, "MThreadState: ");
        pthread_setspecific(gTLS, this);
    }

    MThreadState *MThreadState::self() {
        if (gHaveTLS.load(std::memory_order_acquire)) {
restart:
            const pthread_key_t  k = gTLS;
            MThreadState* st = (MThreadState*) pthread_getspecific(k);// NOLINT
            if (st) return st;
            return new MThreadState;
        }

        pthread_mutex_lock(&gTLSMutex);
        if (!gHaveTLS.load(std::memory_order_relaxed)) {
            int key_create_value = pthread_key_create(&gTLS
                    , reinterpret_cast<void (*)(void *)>(threadDestructor));
            if (key_create_value != 0) {
                pthread_mutex_unlock(&gTLSMutex);
                LOG_W(TAG, "self: unable to create TLS key");
                return nullptr;
            }
            gHaveTLS.store(true, std::memory_order_release);
        }
        pthread_mutex_unlock(&gTLSMutex);
        goto restart;
    }

    void* MThreadState::threadDestructor(void *st) {
        MThreadState* self = static_cast<MThreadState *>(st);
        LOGF_D(TAG, "threadDestructor: (%p)", self);
        delete self;
    }

    MThreadState::~MThreadState() {
        LOG_D(TAG, "~MThreadState: ");
        pthread_key_delete(gTLS);
    }
}


namespace droid {
    class PoolThread : public Thread {
    public:
        ~PoolThread() {
            LOGF_D(TAG, "~PoolThread: (%p)", this);
        }

        explicit PoolThread(bool isMain)
                : Thread(false/*canCallJava*/), mIsMain(isMain) {
            LOGF_D(TAG, "PoolThread: (%p)", this);
        }

    protected:
        bool threadLoop() override {
            pthread_t pthread = getPthreadId();
            pid_t pid = getTid();
            LOGF_D(TAG, "threadLoop: is main(%s)"
                        ", pthread id = %x, pid = %d"
                        , (mIsMain ? "true" : "false")
                        , pthread, pid);
            MThreadState* mThreadState = MThreadState::self();
            sleep(3);
            return false;
        }
        const bool mIsMain;
    };
}

using droid::PoolThread;

int main() {
    LOG_D(TAG, "main: hello droid thread");

/*
    PoolThread* pt = new PoolThread(false);// NOLINT
    pt->run("test");
*/

    (new PoolThread(false))->run("test1");

    sleep(5);

    LOG_D(TAG, "main: end droid thread\n\n\n");
}
