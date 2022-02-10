#ifndef DROID_STRONG_POINTER_H
#define DROID_STRONG_POINTER_H

namespace droid {

    template<typename T>
    class sp {
    public:
        inline sp() : m_ptr(nullptr) {}

        sp(T *other);

        sp(const sp<T> &other);

        ~sp();

    private:
        T *m_ptr;
    };

    template<typename T>
    sp<T>::sp(T *other): m_ptr(other) {
        if (other)
            other->incStrong(this);
    }

    template<typename T>
    sp<T>::sp(const sp<T> &other): m_ptr(other.m_ptr) {
        if (m_ptr)
            m_ptr->incStrong(this);
    }

    template<typename T>
    sp<T>::~sp() {
        if (m_ptr)
            m_ptr->decStrong(this);
    }

} // namespace droid


#endif //DROID_STRONG_POINTER_H