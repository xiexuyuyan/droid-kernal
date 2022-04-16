#include "utils/RefBase.h"

#undef TAG
#define TAG "RefBase.cpp"

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

    ~weakref_impl() {
        LOG_D(TAG, "weakref_impl destructor");
    }
};

void droid::RefBase::incStrong(const void* id) const {
    weakref_impl* const refs = mRefs;

    refs->incWeak(id);// impl in weakref_type

    refs->addStrongRef(id);// debug impl
    const int32_t c = refs->mStrong.fetch_add(
            1, std::memory_order_relaxed);
    LOGF_V(TAG, "incStrong: mStrong.fetch_add = %d", c);
    if (c != INITIAL_STRONG_VALUE) {
        return;
    }

    int32_t old = refs->mStrong.fetch_sub(
            INITIAL_STRONG_VALUE, std::memory_order_relaxed);
    LOGF_V(TAG, "incStrong: mStrong.fetch_sub = %d", old);

    refs->mBase->onFirstRef();
}

void droid::RefBase::decStrong(const void* id) const {
    weakref_impl* const refs = mRefs;

    refs->removeStrongRef(id);
    const int32_t c = refs->mStrong.fetch_sub(
            1, std::memory_order_relaxed);
    LOGF_V(TAG, "decStrong: mStrong.fetch_sub = %d", c);
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

droid::RefBase::~RefBase() {
    int32_t flags = mRefs->mFlags.load(std::memory_order_relaxed);
    if ((flags & OBJECT_LIFETIME_WEAK) == OBJECT_LIFETIME_WEAK) {
        int32_t c = mRefs->mWeak.load(std::memory_order_relaxed);
        if (c == 0) {
            delete mRefs;
        }
    } else if (mRefs->mStrong.load(std::memory_order_relaxed)
                          == INITIAL_STRONG_VALUE) {
        LOG_D(TAG,
              std::string("Explicit destruction, weak count = "
              + std::to_string(mRefs->mWeak)).c_str());
    }
    const_cast<weakref_impl*&>(mRefs) = nullptr;
}

void droid::RefBase::extendObjectLifetime(int32_t mode) {
    mRefs->mFlags.fetch_or(mode, std::memory_order_relaxed);
}
void droid::RefBase::onFirstRef() {}

void droid::RefBase::onLastStrongRef(const void *id) {}

bool droid::RefBase::onIncStrongAttempted(uint32_t flags, const void* id) {
    if (flags & FIRST_INC_STRONG) {
        return true;// default flags=FIRST_INC_STRONG
    } else {
        return false;// not allow to be revived
    }
}

void droid::RefBase::onLastWeakRef(const void* id) { }

droid::RefBase::weakref_type *droid::RefBase::creatWeak(const void *id) const {
    mRefs->incWeak(id);
    return mRefs;
}

droid::RefBase::weakref_type *droid::RefBase::getWeakRefs() const {
    return mRefs;
}

void droid::RefBase::weakref_type::incWeak(const void *id) {
    weakref_impl* const impl = static_cast<weakref_impl*>(this);
    impl->addWeakRef(id);
    const int32_t c = impl->mWeak.fetch_add(
            1, std::memory_order_relaxed);
    LOGF_V(TAG, "incWeak: mWeak.fetch_add = %d", c);
}

void droid::RefBase::weakref_type::decWeak(const void *id) {
    weakref_impl* const impl = static_cast<weakref_impl*>(this);
    impl->removeStrongRef(id);
    const int32_t c = impl->mWeak.fetch_sub(
            1, std::memory_order_relaxed);
    LOGF_V(TAG, "decWeak: mWeak.fetch_sub = %d", c);
    if (c != 1) {
        return;
    }
    std::atomic_thread_fence(std::memory_order_acquire);

    int32_t flags = impl->mFlags.load(std::memory_order_relaxed);
    if ((flags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
        if (impl->mStrong.load(std::memory_order_relaxed)
                        == INITIAL_STRONG_VALUE) {
            // it rarely happened
            LOG_D(TAG,
                  "droid::RefBase::weakref_type::decWeak()"
                  " Object lost last weak reference"
                  " before it had a strong reference.");
        } else {
            delete impl;
        }
    } else {
        // This is the OBJECT_LIFETIME_WEAK case.
        // The last weak reference is gone, we can destroy the object
        impl->mBase->onLastWeakRef(id);
        // in object destructor, when mWeak == 0, we delete the impl
        delete impl->mBase;
    }
}

bool droid::RefBase::weakref_type::attemptIncStrong(const void *id) {
    incWeak(id);

    weakref_impl* const impl = static_cast<weakref_impl*>(this);

    int32_t curCount = impl->mStrong.load(std::memory_order_relaxed);
    LOG_D(TAG, std::string("attemptIncStrong: "
                   "cur strong count = " + std::to_string(curCount)).c_str());
    while (curCount > 0 && curCount != INITIAL_STRONG_VALUE) {
        if (impl->mStrong.compare_exchange_weak(
                curCount, curCount+1, std::memory_order_relaxed)) {
            break;
        }
    }
    if (curCount <= 0 || curCount == INITIAL_STRONG_VALUE) {
        // in this case, there was never a strong reference
        // or all strong reference hava been released
        int32_t flags = impl->mFlags.load(std::memory_order_relaxed);
        if ((flags & OBJECT_LIFETIME_MASK) == OBJECT_LIFETIME_STRONG) {
            if (curCount <= 0) {
                // The last strong-reference got released
                // , the object cannot be revived.
                decWeak(id);
                return false;
            }

            // here, curCount == INITIAL_STRONG_VALUE, which means
            // there never was a strong-reference, so we can try to
            // promote this object; we need to do that atomically.
            while (curCount > 0) {
                if (impl->mStrong.compare_exchange_weak(
                        curCount, curCount+1, std::memory_order_relaxed)) {
                    break;
                }
            }

            if (curCount <= 0) {
                // promote failed, some other thread destroyed us in the
                // meantime (i.e.: strong count reached zero)
                decWeak(id);
                return false;
            }
        } else {
            // This object has an extended life-time
            // i.e.: it can be revived from a weak-reference only
            // Ask the object's implementation if it agrees to be revived
            if (!impl->mBase->onIncStrongAttempted(FIRST_INC_STRONG, id)) {
                decWeak(id);
                return false;
            }
            // grab a strong-reference, which is always safe due to
            // the extended life-time
            curCount = impl->mStrong.fetch_add(1, std::memory_order_relaxed);
            if (curCount != 0 && curCount != INITIAL_STRONG_VALUE) {
                // This case means curCount is < 0
                // , it has already been incremented by someone else
                impl->mBase->onLastStrongRef(id);
            }
        }
    }

    impl->addStrongRef(id);

    if (curCount == INITIAL_STRONG_VALUE) {
        // there never was a strong-reference
        impl->mStrong.fetch_sub(
                INITIAL_STRONG_VALUE, std::memory_order_relaxed);
    }

    return true;
}

bool droid::RefBase::weakref_type::attemptIncWeak(const void *id) {
    weakref_impl* const impl = static_cast<weakref_impl*>(this);

    int32_t curCount = impl->mWeak.load(std::memory_order_relaxed);
    LOGF_ASSERT(curCount > 0
                , "attemptIncWeak: called on %p after underflow"
                , this);
    while (curCount > 0) {
        if (impl->mWeak.compare_exchange_weak(
                curCount, curCount+1, std::memory_order_relaxed)) {
            break;
        }
        // curCount has been updated
    }

    if (curCount > 0) {
        impl->addWeakRef(id);
    }

    return curCount > 0;
}


int32_t droid::RefBase::weakref_type::getWeakCount() const {
    const weakref_impl* const impl = static_cast<const weakref_impl*>(this);
    return impl->mWeak.load(std::memory_order_relaxed);
}
