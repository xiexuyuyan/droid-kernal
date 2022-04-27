#include <thread>
#include "log/log.h"
#include "utils/Looper.h"

#undef TAG
#define TAG "LooperTest.cpp"

using ::droid::sp;
using ::droid::Looper;


class MyHandler : public droid::MessageHandler {
public:
    void handleMessage(const droid::Message &message) override {
        int what = message.what;
        LOGF_D(TAG, "handleMessage: what = %d", what);
    }
};

sp<Looper> looper;
sp<MyHandler> myHandler = new MyHandler;

void clean(void* arg) {
    LOGF_D(TAG, "clean: myHandler count = %d"
    , myHandler->getStrongCount());
}

void threadExec() {
    LOG_D(TAG, "threadExec: new Thread");

    pthread_cleanup_push(clean, (void *) "thread first handler");

        droid::Message message;
        for (int i = 0; i < 1; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            message.what = 10000 + i;
            looper->sendMessage(myHandler, message);
        }

    pthread_cleanup_pop(1);
}

int main() {
    LOG_D(TAG, "main: start");

    LOGF_D(TAG, "main: myHandler count = %d"
    , myHandler->getStrongCount());

    std::thread newThread(threadExec);
    newThread.detach();
    looper = Looper::prepare(false);
    while (true) {
        looper->pollAll(-1);
    }

    LOG_D(TAG, "main: \n\n\n");
}
