#ifndef DROID_REF_BASE_H
#define DROID_REF_BASE_H

#include <atomic>

#include "log/log.h"
#include "StrongPointer.h"

#undef TAG
#define TAG "RefBase.h"

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

            // acquire a strong reference if there is already one
            bool attemptIncStrong(const void* id);
            bool attemptIncWeak(const void* id);

            // debug only
            int32_t getWeakCount() const;
        };

        weakref_type* creatWeak(const void* id) const;
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

        // Flags for onIncStrongAttempted()
        enum {
            FIRST_INC_STRONG = 0x0001
        };

        void extendObjectLifetime(int32_t mode);

        virtual void onFirstRef();
        virtual void onLastStrongRef(const void* id);
        virtual bool onIncStrongAttempted(uint32_t flags, const void* id);
        virtual void onLastWeakRef(const void* id);

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
            LOG_D(TAG, "incStrong");
            __sync_fetch_and_add(&mCount, 1);
        }

        inline void decStrong(const void *id) const {
            LOG_D(TAG, "decStrong");
            if (__sync_fetch_and_sub(&mCount, 1) == 1) {
                LOG_D(TAG, "delete");
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
    template <typename T>
    class wp {
    public:
        typedef typename droid::RefBase::weakref_type weakref_type;

        inline wp(): m_ptr(0){}

        explicit wp(T* other);
        wp(const wp<T>& other);

        ~wp();

        // promotion to sp
        droid::sp<T> promote() const;
    private:
        template<typename Y> friend class sp;
        template<typename Y> friend class wp;
        T* m_ptr;
        weakref_type* m_refs;
    };

    template<typename T>
    wp<T>::wp(T* other): m_ptr(other) {
        if (other)
            m_refs = other->creatWeak(this);
    }

    template<typename T>
    wp<T>::wp(const wp<T> &other): m_ptr(other.m_ptr), m_refs(other.m_refs) {
        if (m_ptr)
            m_refs->incWeak(this);
    }

    template<typename T>
    wp<T>::~wp() {
        LOG_D(TAG, "~wp: ");
        if (m_ptr)
            m_refs->decWeak(this);
    }

    template<typename T>
    sp<T> wp<T>::promote() const {
        sp<T> result;
        if (m_ptr && m_refs->attemptIncStrong(&result)) {
            result.set_pointer(m_ptr);
        }
        return result;
    }

    // ---------------------------------------------------------------------------

} // namespace droid

#endif //DROID_REF_BASE_H