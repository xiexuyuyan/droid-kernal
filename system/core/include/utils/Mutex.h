#ifndef LIB_UTILS_MUTEX_H
#define LIB_UTILS_MUTEX_H

#include <sys/types.h>
#include <mutex>

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
        int32_t lock() ACQUIRE();
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
        std::mutex mutex;
    };

    typedef Mutex::Autolock AutoMutex;
} // namespace droid

#endif // LIB_UTILS_MUTEX_H