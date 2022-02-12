#ifndef DROID_STRONG_POINTER_H
#define DROID_STRONG_POINTER_H

#include "log/log.h"

#undef LOG_TAG
#define LOG_TAG "StrongPointer.h"

namespace droid {

    template<typename T>
    class sp {
    public:
        inline sp() : m_ptr(nullptr) {}

        sp(T* other);

        sp(const sp<T>& other);

        ~sp();

        // accessors
        inline T* operator-> () const { return m_ptr; }
        inline T* get ()        const { return m_ptr; }

        void set_pointer(T* ptr);
    private:
        T* m_ptr;
    };

    template<typename T>
    sp<T>::sp(T* other): m_ptr(other) {
        if (other)
            other->incStrong(this);
    }

    template<typename T>
    sp<T>::sp(const sp<T>& other): m_ptr(other.m_ptr) {
        if (m_ptr)
            m_ptr->incStrong(this);
    }

    template<typename T>
    sp<T>::~sp() {
        LOG_D(LOG_TAG, "~sp: ");
        if (m_ptr)
            m_ptr->decStrong(this);
    }

    template<typename T>
    void sp<T>::set_pointer(T* ptr) {
        m_ptr = ptr;
    }

} // namespace droid


#endif //DROID_STRONG_POINTER_H