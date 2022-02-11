#ifndef DROID_REF_BASE_H
#define DROID_REF_BASE_H

#include <atomic>

#include "log/log.h"

namespace droid {
    // ---------------------------------------------------------------------------

    class RefBase {
    public:
        void incStrong(const void *id) const;
        void decStrong(const void *id) const;

        // debug only
        int32_t getStrongCount() const;

        class weakref_type {
        public:
            void incWeak(const void* id);
            void decWeak(const void* id);

            // debug only
            int32_t getWeakCount() const;
        };

        weakref_type* getWeakRefs() const;

    protected:
        RefBase();
        virtual ~RefBase();

        // Flags for extend object lifetime
        enum {
            OBJECT_LIFETIME_STRONG = 0x0000,
            OBJECT_LIFETIME_WEAK   = 0x0001,
            OBJECT_LIFETIME_MASK   = 0x0001
        };

        virtual void onFirstRef();
        virtual void onLastStrongRef(const void* id);

    private:
        class weakref_impl;
        weakref_impl* const mRefs;
    };

    // ---------------------------------------------------------------------------

    template<class T>
    class LightRefBase {
    public:
        inline LightRefBase() : mCount(0) {}

        inline void incStrong(const void *id) const {
            LOG_D("LightRefBase", "incStrong");
            __sync_fetch_and_add(&mCount, 1);
        }

        inline void decStrong(const void *id) const {
            LOG_D("LightRefBase", "decStrong");
            if (__sync_fetch_and_sub(&mCount, 1) == 1) {
                LOG_D("LightRefBase", "delete");
                delete static_cast<const T *>(this);
            }
        }

        inline int32_t getStrongCount() const {
            return mCount;
        }

    protected:
        inline ~LightRefBase() = default;

    private:
        mutable volatile int32_t mCount;
    };
    // ---------------------------------------------------------------------------

} // namespace droid

#endif //DROID_REF_BASE_H