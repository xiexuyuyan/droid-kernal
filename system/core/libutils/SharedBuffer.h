#ifndef DROID_SHARED_BUFFER_H
#define DROID_SHARED_BUFFER_H

#include <sys/types.h>
#include <atomic>

#include "log/log.h"
#undef TAG
#define TAG "SharedBuffer.h"

namespace droid {
    class SharedBuffer {
    public:
        enum {
            eKeepStorage = 0x00000001,
        };

        static              SharedBuffer* alloc(size_t size);
        static              void          dealloc(const SharedBuffer* released);
               inline const void*         data() const;
               inline       void*         data();
               inline       size_t        size() const;
        static inline       SharedBuffer* bufferFromData(void *data);
        static inline const SharedBuffer* bufferFromData(const void *data);
                            SharedBuffer* editResize(size_t size) const;
                            SharedBuffer* attemptEdit() const;
                            int32_t       release(uint32_t flags = 0) const;
               inline       bool          onlyOwner() const;
    private:
        mutable std::atomic<int32_t>      mRefs;
                            size_t        mSize;
    public:
                            uint32_t      mClientMetaData;
    };

    const void* SharedBuffer::data() const {
        return this + 1;
    }
    void* SharedBuffer::data() {
        return this + 1;
    }

    size_t SharedBuffer::size() const {
        return mSize;
    }

    SharedBuffer *SharedBuffer::bufferFromData(void *data) {
        return data ? static_cast<SharedBuffer*>(data) - 1 : nullptr;
    }
    const SharedBuffer *SharedBuffer::bufferFromData(const void *data) {
        return data
            ? static_cast<const SharedBuffer*>(data) - 1 : nullptr;
    }

    bool SharedBuffer::onlyOwner() const {
        return (mRefs.load(std::memory_order_acquire) == 1);
    }
} // namespace droid

#endif // DROID_SHARED_BUFFER_H