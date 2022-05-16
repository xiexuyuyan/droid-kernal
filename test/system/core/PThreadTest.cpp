#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_key_t   key;
void echomsg(int t)
{
    printf("destructor excuted in thread %d,param=%d\n",pthread_self(),t);
}
void * child1(void *arg)
{
    int tid=pthread_self();
    printf("thread %d enter\n",tid);
    pthread_setspecific(key,(void *)tid);
    sleep(2);
    printf("thread %d returns %d\n",tid,pthread_getspecific(key));
    sleep(5);
}
void * child2(void *arg)
{
    int tid=pthread_self();
    printf("thread %d enter\n",tid);
    pthread_setspecific(key,(void *)tid);
    sleep(1);
    printf("thread %d returns %d\n",tid,pthread_getspecific(key));
    sleep(5);
}
int main(void)
{
    int tid1,tid2;
    printf("hello\n");
    pthread_key_create(&key, reinterpret_cast<void (*)(void *)>(echomsg));
    pthread_create(reinterpret_cast<pthread_t *>(&tid1), NULL, child1, NULL);
    pthread_create(reinterpret_cast<pthread_t *>(&tid2), NULL, child2, NULL);
    sleep(10);
    pthread_key_delete(key);
    printf("main thread exit\n");
    return 0;
}