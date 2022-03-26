#ifndef DROID_SHARED_BUFFER_H
#define DROID_SHARED_BUFFER_H

#include <sys/types.h>

#include "log/log.h"
#undef TAG
#define TAG "SharedBuffer.h"

namespace droid {
    class SharedBuffer {
    public:
        static SharedBuffer* alloc(size_t size);

        inline const void* data() const;
        inline       void* data();
    };

    const void* SharedBuffer::data() const {
        return this + 1;
    }
    void* SharedBuffer::data() {
        return this + 1;
    }
} // namespace droid

#endif // DROID_SHARED_BUFFER_H