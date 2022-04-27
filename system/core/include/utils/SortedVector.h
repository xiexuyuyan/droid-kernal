#ifndef DROID_SORTED_VECTOR_H
#define DROID_SORTED_VECTOR_H

#include "utils/Vector.h"
#include "utils/VectorImpl.h"
#include "log/log.h"

#undef TAG
#define TAG "SortedVector.h"

namespace droid {
    template<class TYPE>
    class SortedVector : private SortedVectorImpl {
    public:
                                SortedVector();

        inline  size_t          size() const { return VectorImpl::size(); }
        inline  bool            isEmpty() const { return VectorImpl::isEmpty(); }
        inline  const TYPE*     array() const;
                ssize_t         indexOf(const TYPE& item) const;
        inline  const TYPE&     operator [] (size_t index) const;
        inline  const TYPE&     itemAt(size_t index) const;

    private:
        virtual void    do_construct(void *storage, size_t num) const;
        virtual void    do_destroy(void *storage, size_t num) const;
        virtual void    do_copy(void *dest, const void *from, size_t num) const;
        virtual void    do_splat(void *dest, const void *from, size_t num) const;
        virtual void    do_move_forward(void *dest, const void *from, size_t num) const;
        virtual int     do_compare(const void* lhs, const void* rhs) const;
    };


    template<class TYPE> inline
    const TYPE* SortedVector<TYPE>::array() const {
        return static_cast<const TYPE*>(arrayImpl());
    }

    template<class TYPE> inline
    ssize_t SortedVector<TYPE>::indexOf(const TYPE& item) const {
        return SortedVectorImpl::indexOf(&item);
    }

    template<class TYPE> inline
    const TYPE &SortedVector<TYPE>::operator[](size_t index) const {
        LOGF_ASSERT(index >= size(), "operator[]: "
            "%s: index=%u out of range (%u)"
            , __PRETTY_FUNCTION__
            , int(index), int(size()));
        return *(array() + index);
    }

    template<class TYPE> inline
    const TYPE &SortedVector<TYPE>::itemAt(size_t index) const {
        return operator[](index);
    }

    template<class TYPE>
    UTILS_VECTOR_NO_CFI void SortedVector<TYPE>::do_construct(void *storage, size_t num) const {
        construct_type(reinterpret_cast<TYPE*>(storage) ,num);
    }

    template<class TYPE>
    UTILS_VECTOR_NO_CFI void SortedVector<TYPE>::do_destroy(void *storage, size_t num) const {
        destroy_type(reinterpret_cast<TYPE*>(storage), num);
    }

    template<class TYPE>
    UTILS_VECTOR_NO_CFI void SortedVector<TYPE>::do_copy(void *dest, const void *from, size_t num) const {
        copy_type(reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(from), num);
    }

    template<class TYPE>
    UTILS_VECTOR_NO_CFI void SortedVector<TYPE>::do_splat(void *dest, const void *from, size_t num) const {
        splat_type(reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(from), num);
    }

    template<class TYPE>
    UTILS_VECTOR_NO_CFI void SortedVector<TYPE>::do_move_forward(void *dest, const void *from, size_t num) const {
        move_forward_type(reinterpret_cast<TYPE*>(dest), reinterpret_cast<const TYPE*>(from), num);
    }

    template<class TYPE>
    int SortedVector<TYPE>::do_compare(const void *lhs, const void *rhs) const {
        return compare_type(*reinterpret_cast<const TYPE*>(lhs), *reinterpret_cast<const TYPE*>(rhs));
    }

    template<class TYPE> inline
    SortedVector<TYPE>::SortedVector()
        : SortedVectorImpl(sizeof(TYPE)
            , ((traits<TYPE>::has_trivial_ctor ? HAS_TRIVIAL_CTOR : 0)
            | (traits<TYPE>::has_trivial_dtor ? HAS_TRIVIAL_DTOR : 0)
            | (traits<TYPE>::has_trivial_copy ? HAS_TRIVIAL_COPY : 0)
            )) {
    }

} // namespace droid

#endif // DROID_SORTED_VECTOR_H