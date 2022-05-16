#ifndef UNTITLED_RUNNABLETHREAD_H
#define UNTITLED_RUNNABLETHREAD_H


#include "utils/Looper.h"

using ::droid::sp;
using droid::MessageHandler;
using droid::Looper;

class RunnableThread {
public:
    explicit RunnableThread(
            const sp<Looper>& looper, const sp<MessageHandler>& handler);
    ~RunnableThread();
    void run();
    static void self();
private:
    droid::sp<MessageHandler> mHandler;
    droid::sp<Looper> mLooper;

    static void threadDestructor(void* st);
};



#endif //UNTITLED_RUNNABLETHREAD_H
