#ifndef DROID_VECTOR_H
#define DROID_VECTOR_H

#include "utils/VectorImpl.h"

namespace droid {
    template<class TYPE>
    class Vector : private VectorImpl {
    public:
        Vector();

        inline size_t size() const override { return VectorImpl::size(); }
    };
    template<class TYPE> inline
    Vector<TYPE>::Vector()
        : VectorImpl(sizeof(TYPE), 0) {
        // todo(20220326-171257 handle the traits!!!)
    }
} // namespace droid

#endif // DROID_VECTOR_H