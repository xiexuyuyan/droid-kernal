#include "utils/VectorImpl.h"

#undef TAG
#define TAG "VectorImpl.cpp"

namespace droid {

    VectorImpl::VectorImpl(size_t itemSize, uint32_t flags)
        : mStorage(nullptr)
        , mCount(0)
        , mFlags(flags)
        , mItemSize(itemSize) {

    }
} // namespace droid