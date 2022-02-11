#define LOG_TAG "RefBase.cpp"

#include "utils/RefBase.h"

#define INITIAL_STRONG_VALUE (1<<28)

class droid::RefBase::weakref_impl : public droid::RefBase::weakref_type {
public:
    std::atomic<int32_t> mStrong;
    std::atomic<int32_t> mWeak;
    RefBase* const       mBase;
    std::atomic<int32_t> mFlags;

    explicit weakref_impl(RefBase* base)
    : mStrong(INITIAL_STRONG_VALUE)
    , mWeak(0)
    , mBase(base)
    , mFlags(0){}

    void addStrongRef(const void* /*id*/){}
    void removeStrongRef(const void* /*id*/){}
    void addWeakRef(const void* /*id*/){}
    void removeWeakRef(const void* /*id*/){}
};

void droid::RefBase::incStrong(const void* id) const {
    weakref_impl* const refs = mRefs;

    refs->incWeak(id);// impl in weakref_type

    refs->addStrongRef(id);// debug impl
    const int32_t c = refs->mStrong.fetch_add(
            1, std::memory_order_relaxed);
    LOG_D(LOG_TAG,
          "droid::RefBase::incStrong():"
          "mStrong.fetch_add = "
          + std::to_string(c));
    if (c != INITIAL_STRONG_VALUE) {
        return;
    }

    int32_t old = refs->mStrong.fetch_sub(
            INITIAL_STRONG_VALUE, std::memory_order_relaxed);
    LOG_D(LOG_TAG,
          "droid::RefBase::incStrong():"
          "mStrong.fetch_sub(INITIAL_STRONG_VALUE) = "
          + std::to_string(old));

    refs->mBase->onFirstRef();
}

void droid::RefBase::decStrong(const void* id) const {
    weakref_impl* const refs = mRefs;

    refs->removeStrongRef(id);
    const int32_t c = refs->mStrong.fetch_sub(
            1, std::memory_order_relaxed);
    LOG_D(LOG_TAG,
          "droid::RefBase::decStrong():"
          "mStrong.fetch_sub = "
          + std::to_string(c));
    if (c == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        refs->mBase->onLastStrongRef(id);
        int32_t flags = refs->mFlags.load(std::memory_order_relaxed);
        if ((flags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
            delete this;
            // The destructor does not delete refs in the case.
        }
    }
    refs->decWeak(id);
}

int32_t droid::RefBase::getStrongCount() const {
    // debug only
    return mRefs->mStrong.load(std::memory_order_relaxed);
}


droid::RefBase::RefBase(): mRefs(new weakref_impl(this)){}

void droid::RefBase::onFirstRef() {}

void droid::RefBase::onLastStrongRef(const void *id) {}

droid::RefBase::~RefBase() {
    int32_t flags = mRefs->mFlags.load(std::memory_order_relaxed);
    if ((flags & OBJECT_LIFETIME_WEAK) == OBJECT_LIFETIME_WEAK) {
        if (mRefs->mWeak.load(std::memory_order_relaxed)) {
            delete mRefs;
        }
    } else if (mRefs->mStrong.load(std::memory_order_relaxed)
                          == INITIAL_STRONG_VALUE) {
        LOG_D(LOG_TAG,
              "Explicit destruction, weak count = "
              + std::to_string(mRefs->mWeak));
    }
    const_cast<weakref_impl*&>(mRefs) = nullptr;
}

droid::RefBase::weakref_type *droid::RefBase::getWeakRefs() const {
    return mRefs;
}

void droid::RefBase::weakref_type::incWeak(const void *id) {
    weakref_impl* const impl = static_cast<weakref_impl*>(this);
    impl->addWeakRef(id);
    const int32_t c = impl->mWeak.fetch_add(
            1, std::memory_order_relaxed);
    LOG_D(LOG_TAG,
          "droid::RefBase::weakref_type::incWeak():"
          "mWeak.fetch_add = "
          + std::to_string(c));
}

void droid::RefBase::weakref_type::decWeak(const void *id) {
    weakref_impl* const impl = static_cast<weakref_impl*>(this);
    impl->removeStrongRef(id);
    const int32_t c = impl->mWeak.fetch_sub(
            1, std::memory_order_relaxed);
    LOG_D(LOG_TAG,
          "droid::RefBase::weakref_type::decWeak():"
          "mWeak.fetch_sub = "
          + std::to_string(c));
    if (c != 1) {
        return;
    }
    std::atomic_thread_fence(std::memory_order_acquire);

    int32_t flags = impl->mFlags.load(std::memory_order_relaxed);
    if ((flags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
        if (impl->mStrong.load(std::memory_order_relaxed)
                        == INITIAL_STRONG_VALUE) {
            // it rarely happened
            LOG_D(LOG_TAG,
                  "droid::RefBase::weakref_type::decWeak()"
                  " Object lost last weak reference"
                  " before it had a strong reference.");
        } else {
            delete impl;
        }
    } else {
        // This is the OBJECT_LIFETIME_WEAK case.
        // The last weak reference is gone, we can destroy the object
        delete impl->mBase;
    }
}

int32_t droid::RefBase::weakref_type::getWeakCount() const {
    const weakref_impl* const impl = static_cast<const weakref_impl*>(this);
    return impl->mWeak.load(std::memory_order_relaxed);
}
