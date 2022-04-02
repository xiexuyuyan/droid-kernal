#ifndef DROID_STRONG_POINTER_H
#define DROID_STRONG_POINTER_H

#include "log/log.h"

#undef TAG
#define TAG "StrongPointer.h"

namespace droid {

    template<typename T>
    class sp {
    public:
        inline sp() : m_ptr(nullptr) {}

        sp(T* other);
        sp(const sp<T>& other);
        template<typename U> sp(const sp<U>& other);

        ~sp();

        sp& operator = (T* other);

        // accessors
        inline T* operator-> () const { return m_ptr; }
        inline T* get ()        const { return m_ptr; }
        // todo(how to override operator != and ==)
        inline bool operator != (const void* p) const { return m_ptr != p; }
        inline bool operator == (const void* p) const { return m_ptr == p; }

    private:
        template<typename Y> friend class sp;
        template<typename Y> friend class wp;
        void set_pointer(T* ptr);
        T* m_ptr;
    };

    template<typename T>
    sp<T>::sp(T* other): m_ptr(other) {
        LOG_V(TAG, "sp(T*): ");
        if (other)
            other->incStrong(this);
    }

    template<typename T>
    sp<T>::sp(const sp<T>& other): m_ptr(other.m_ptr) {
        LOG_V(TAG, "sp(sp<T>&): ");
        if (m_ptr)
            m_ptr->incStrong(this);
    }

    template<typename T> template<typename U>
    sp<T>::sp(const sp<U>& other) : m_ptr(other.m_ptr) {
        if (m_ptr)
            m_ptr->incStrong(this);
    }

    template<typename T>
    sp<T>::~sp() {
        LOG_V(TAG, "~sp: ");
        if (m_ptr)
            m_ptr->decStrong(this);
    }

    template<typename T>
    void sp<T>::set_pointer(T* ptr) {
        m_ptr = ptr;
    }

    template<typename T>
    sp<T>& sp<T>::operator =(T* other) {
        T* oldPtr(*const_cast<T* volatile*>(&m_ptr));
        if (other) {
            // todo(do some PAGE check, in 4K)
            LOG_V(TAG, "operator=: increase strong refs");
            other->incStrong(this);
        }
        if (oldPtr) oldPtr->decStrong(this);
        if (oldPtr != *const_cast<T* volatile*>(&m_ptr))
            // todo(why? this appears in android source)
            ;
        m_ptr = other;
        return *this;
    }
} // namespace droid


#endif //DROID_STRONG_POINTER_H