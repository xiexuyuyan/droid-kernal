#ifndef DROID_VECTOR_H
#define DROID_VECTOR_H

#include "utils/VectorImpl.h"
#include "utils/TypeHelpers.h"
#include "log/log.h"

#undef TAG
#define TAG "Vector.h"

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(no_sanitize)
#define UTILS_VECTOR_NO_CFI __attribute__((no_sanitize("cfi")))
#else
#define UTILS_VECTOR_NO_CFI
#endif

namespace droid {
    template<class TYPE>
    class Vector : private VectorImpl {
    public:
        Vector();

        inline  void            clear() { VectorImpl::clear(); }
        inline  size_t          size() const override { return VectorImpl::size(); }
        inline  bool            isEmpty() const { return VectorImpl::isEmpty(); }
        inline  size_t          capacity() const { return VectorImpl::capacity(); }
        inline  ssize_t         setCapacity(size_t size) { return VectorImpl::setCapacity(size); }
        inline  const TYPE*     array() const;
        inline  const TYPE&     operator [] (size_t index) const;
        inline  const TYPE&     itemAt(size_t index) const;
                TYPE&           editItemAt(size_t index);
                ssize_t         insertAt(const TYPE& prototype_item
                                         , size_t index
                                         , size_t numItems = 1);
        // todo(20220327-164929 we mark the inline tag at definition but not in assignment, why ?)
        inline  void            push();
                void            push(const TYPE& item);
        inline  ssize_t         removeItemsAt(size_t index, size_t count = 1);
        inline  ssize_t         removeAt(size_t index) { return removeItemsAt(index); }

    protected:
        virtual void do_construct(void* storage, size_t num) const;
        virtual void do_destroy(void* storage, size_t num) const;
        virtual void do_copy(void* dest, const void* from, size_t num) const;
        virtual void do_splat(void *dest, const void *from, size_t num) const;
        virtual void do_move_forward(void *dest, const void *from, size_t num) const;
    };
    template<class TYPE> inline
    Vector<TYPE>::Vector() : VectorImpl(
            sizeof(TYPE)
            , ((traits<TYPE>::has_trivial_ctor ? HAS_TRIVIAL_CTOR : 0)
            | (traits<TYPE>::has_trivial_dtor ? HAS_TRIVIAL_DTOR : 0)
            | (traits<TYPE>::has_trivial_copy ? HAS_TRIVIAL_COPY : 0))) {
        // todo(20220326-171257 handle the traits!!!)
    }

    template<class TYPE>
    const TYPE *Vector<TYPE>::array() const {
        return static_cast<const TYPE*>(arrayImpl());
    }

    template<class TYPE>
    const TYPE &Vector<TYPE>::operator[](size_t index) const {
        LOGF_ASSERT(index < size(), "operator[]"
            ": %s: index=%u out of range (%u)"
            , __PRETTY_FUNCTION__
            , int(index), int(size()));
        return *(array()+ index);
    }

    template<class TYPE>
    const TYPE &Vector<TYPE>::itemAt(size_t index) const {
        return operator[](index);
    }

    template<class TYPE>
    TYPE &Vector<TYPE>::editItemAt(size_t index) {
        return *( static_cast<TYPE*>(editItemLocation(index)) );
    }

    template<class TYPE> inline
    ssize_t Vector<TYPE>::insertAt(
            const TYPE& item
            , size_t index
            , size_t numItems) {
        return VectorImpl::insertAt(&item, index, numItems);
    }

    template<class TYPE> inline
    void Vector<TYPE>::push() {
        VectorImpl::push();
    }

    template<class TYPE> inline
    void Vector<TYPE>::push(const TYPE& item) {
        return VectorImpl::push(&item);
    }

    template <class TYPE>
    UTILS_VECTOR_NO_CFI void Vector<TYPE>::do_construct(
            void *storage, size_t num) const {
        construct_type(reinterpret_cast<TYPE*>(storage), num);
    }

    template <class TYPE>
    UTILS_VECTOR_NO_CFI void Vector<TYPE>::do_destroy(
            void *storage, size_t num) const {
        destroy_type(reinterpret_cast<TYPE*>(storage), num);
    }

    template<class TYPE>
    UTILS_VECTOR_NO_CFI void Vector<TYPE>::do_copy(
            void *dest, const void *from, size_t num) const {
        copy_type(reinterpret_cast<TYPE*>(dest)
                  , reinterpret_cast<const TYPE*>(from), num);
    }

    template <class TYPE>
    UTILS_VECTOR_NO_CFI void Vector<TYPE>::do_splat(
            void *dest, const void *from, size_t num) const {
        splat_type(reinterpret_cast<TYPE*>(dest)
                , reinterpret_cast<const TYPE*>(from), num);
    }

    template <class TYPE>
    UTILS_VECTOR_NO_CFI void Vector<TYPE>::do_move_forward(
            void *dest, const void *from, size_t num) const {
        move_forward_type(reinterpret_cast<TYPE*>(dest)
                          , reinterpret_cast<const TYPE*>(from), num);
    }

    template<class TYPE> inline
    ssize_t Vector<TYPE>::removeItemsAt(size_t index, size_t count) {
        return VectorImpl::removeItemsAt(index, count);
    }

} // namespace droid

#endif // DROID_VECTOR_H