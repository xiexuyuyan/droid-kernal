#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "log.h"

#undef TAG
#define TAG "MPThreadTest.cpp"


class A {
public:
    static void self() {
        LOG_D(TAG, "sa: ");
    }
    A() {
        LOG_D(TAG, "A: ");
    }
};

pthread_key_t key;

void* threadDestructor(void* st) {
    LOG_D(TAG, "threadDestructor: ");
}

void* timer(void* st) {
    int tid = pthread_self();
    pthread_setspecific(key,(void *)&tid);
    LOGF_D(TAG, "timer: tid = %d", tid);
    sleep(1);
}

int main() {
    int tid1, tid2;
    pthread_key_create(&key
            , reinterpret_cast<void (*)(void *)>(threadDestructor));
    pthread_create(
            reinterpret_cast<pthread_t *>(&tid1), NULL, timer, NULL);
    sleep(2);
    pthread_key_delete(key);
    return 0;
}