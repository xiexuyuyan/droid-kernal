//
// Created by 23517 on 2022/5/10.
//
#include <thread>

#include "RunnableThread.h"
#include "log/log.h"

#undef TAG
#define TAG "RunnableThread.cpp"

RunnableThread::RunnableThread(
        const sp<Looper>& looper, const sp<MessageHandler>& handler) {
    mHandler = handler;
    mLooper = looper;
}

static pthread_key_t gTSL = 0;
void RunnableThread::self() {
    int key_create_value =
            pthread_key_create(&gTSL, threadDestructor);
    LOGF_D(TAG, "self: key_create_value = %d", key_create_value);
}


void RunnableThread::run() {
    droid::Message message;
    for (int i = 0; i < 1; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        message.what = 10000 + i;
        LOGF_D(TAG, "threadExec: send message what = %d"
                , message.what);
        mLooper->sendMessage(mHandler, message);
    }
}

RunnableThread::~RunnableThread() {
    LOG_D(TAG, "~RunnableThread: ");
}
void RunnableThread::threadDestructor(void* st) {
    LOG_D(TAG, "---threadDestructor: ---");
}

