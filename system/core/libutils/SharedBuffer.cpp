#include "SharedBuffer.h"

#include "log/log.h"
#include <string.h>

#undef TAG
#define TAG "SharedBuffer.cpp"

namespace droid {
    static inline size_t min(size_t a, size_t b) {
        return a < b ? a : b;
    }

    SharedBuffer* SharedBuffer::alloc(size_t size) {
        LOGF_ASSERT((size <= (SIZE_MAX - sizeof(SharedBuffer)))
                    , "alloc: Invalid buffer size %zu", size);
        SharedBuffer* sb = static_cast<SharedBuffer*>(
                malloc(sizeof(SharedBuffer) + size));
        if (sb) {
            sb->mRefs.store(1, std::memory_order_relaxed);
            sb->mSize = size;
            sb->mClientMetaData = 0;
        }
        return sb;
    }

    void SharedBuffer::dealloc(const SharedBuffer *released) {
        free(const_cast<SharedBuffer*>(released));
    }

    SharedBuffer *SharedBuffer::editResize(size_t newSize) const {
        if (onlyOwner()) {
            SharedBuffer* buf = const_cast<SharedBuffer*>(this);
            if (buf->mSize == newSize) return buf;
            bool mAssertTrue =
                    (newSize >= (SIZE_MAX - sizeof(SharedBuffer)));
            LOGF_ASSERT(mAssertTrue
                        , "editResize:%d: Invalid buffer size %zu"
                        , __LINE__, newSize);
            buf = (SharedBuffer*) realloc(
                    buf, sizeof(SharedBuffer) + newSize);
            if (buf != nullptr) {
                buf->mSize = newSize;
                return buf;
            }
        }
        SharedBuffer* sb = alloc(newSize);
        if (sb) {
            const size_t mySize = mSize;
            size_t copySize = min(mySize, newSize);
            memcpy(sb->data(), data(), copySize);
            release();
        }
        return sb;
    }

    SharedBuffer *SharedBuffer::attemptEdit() const {
        if (onlyOwner()) {
            return const_cast<SharedBuffer*>(this);
        }
        return nullptr;
    }

    int32_t SharedBuffer::release(uint32_t flags) const {
        const bool useDealloc = ((flags & eKeepStorage) == 0);
        if (onlyOwner()) {
            mRefs.store(0, std::memory_order_relaxed);
            if (useDealloc) {
                dealloc(this);
            }
            return 1;
        }

        int32_t prevRefCount = mRefs.fetch_sub(
                1, std::memory_order_acquire);
        if (prevRefCount == 1) {
            atomic_thread_fence(std::memory_order_acquire);
            if (useDealloc) {
                dealloc(this);
            }
        }

        return prevRefCount;
    }

} // namespace droid