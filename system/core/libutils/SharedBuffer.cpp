#include "SharedBuffer.h"

namespace droid {
    SharedBuffer* SharedBuffer::alloc(size_t size) {
        LOGF_ASSERT((size <= (SIZE_MAX - sizeof(SharedBuffer)))
                    , "alloc: Invalid buffer size %zu", size);
        SharedBuffer* sb = static_cast<SharedBuffer*>(
                malloc(sizeof(SharedBuffer) + size));
        // todo(20220325-194842 handle the refs saved)
        return sb;
    }

} // namespace droid