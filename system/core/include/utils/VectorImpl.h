#ifndef DROID_VECTOR_IMPL_H
#define DROID_VECTOR_IMPL_H

#include <stdint.h>
#include <sys/types.h>

namespace droid {
    class VectorImpl {
    public:

        VectorImpl(size_t itemSize, uint32_t flags);

        virtual inline size_t size() const { return mCount; }

    private:
        void* mStorage;
        size_t mCount;
        const uint32_t mFlags;
        const size_t mItemSize;
    };
} // namespace droid

#endif // DROID_VECTOR_IMPL_H