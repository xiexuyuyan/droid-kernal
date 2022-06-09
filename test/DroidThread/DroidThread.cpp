#include "utils/Thread.h"
#include "log/log.h"

#include <pthread.h>
#include <unistd.h>


#undef TAG
#define TAG "DroidThread.cpp"


namespace droid {
    class PoolThread : public Thread {
    public:
        explicit PoolThread(bool isMain)
                : Thread(false/*canCallJava*/), mIsMain(isMain) {}

    protected:
        bool threadLoop() override {
            LOGF_D(TAG, "threadLoop: is main :%s", (mIsMain ? "true" : "false"));
            sleep(30);
            return true;
        }
        const bool mIsMain;
    };
}

using droid::PoolThread;

int main() {
    LOG_D(TAG, "main: hello droid thread");

    PoolThread* pt = new PoolThread(false);// NOLINT
    pt->run("test");
    sleep(1);
    pthread_t pthread = pt->getPthreadId();
    pid_t pid = pt->getTid();
    LOGF_D(TAG, "main: pthread id = %x, pid = %x", pthread, pid);

    sleep(30);

    LOG_D(TAG, "main: end droid thread\n\n\n");
}
