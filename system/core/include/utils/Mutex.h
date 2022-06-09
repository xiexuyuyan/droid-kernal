#ifndef LIB_UTILS_MUTEX_H
#define LIB_UTILS_MUTEX_H

#include <sys/types.h>
#include <mutex>
#include "utils/Errors.h"

#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x) // no-op
#endif

#define CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(capability(x))
#define SCOPED_CAPABILITY THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)
#define ACQUIRE(...) THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))
#define RELEASE(...) THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

namespace droid {
    class CAPABILITY("mutex") Mutex {
    public:
        enum {
            PRIVATE = 0,
            SHARED = 1
        };

        Mutex();
        explicit Mutex(const char* name);
/*
        explicit Mutex(int type, const char* name = nullptr);
*/
        ~Mutex();

        status_t lock() ACQUIRE();
        void unlock() RELEASE();

        class SCOPED_CAPABILITY Autolock {
        public:
            inline explicit Autolock(Mutex& mutex) ACQUIRE(mutex) : mlock(mutex) { mlock.lock(); }
            inline explicit Autolock(Mutex* mutex) ACQUIRE(*mutex) : mlock(*mutex) { mlock.lock(); }
            inline ~Autolock() RELEASE() { mlock.unlock(); }
        private:
            Mutex& mlock;
        };

    private:
        pthread_mutex_t mMutex;
    };

    typedef Mutex::Autolock AutoMutex;

    inline Mutex::Mutex() {
        pthread_mutex_init(&mMutex, nullptr);
    }

    inline Mutex::Mutex(__attribute__((unused)) const char *name) {
        pthread_mutex_init(&mMutex, nullptr);
    }
/*
    inline Mutex::Mutex(int type, const char *name) {
        if (type == SHARED) {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(&mMutex, &attr);
            pthread_mutexattr_destroy(&attr);
        } else {
            pthread_mutex_init(&mMutex, nullptr);
        }
    }
*/
    inline Mutex::~Mutex() {
        pthread_mutex_destroy(&mMutex);
    }

    inline status_t Mutex::lock() {
        return -pthread_mutex_lock(&mMutex);
    }

    inline void Mutex::unlock() {
        pthread_mutex_unlock(&mMutex);
    }

} // namespace droid

#endif // LIB_UTILS_MUTEX_H