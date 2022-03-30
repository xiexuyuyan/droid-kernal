#ifndef DROID_VECTOR_IMPL_H
#define DROID_VECTOR_IMPL_H

#include <stdint.h>
#include <sys/types.h>
#include "utils/Errors.h"

namespace droid {
    class VectorImpl {
    public:
        enum {
            HAS_TRIVIAL_CTOR = 0x00000001,
            HAS_TRIVIAL_DTOR = 0x00000002,
            HAS_TRIVIAL_COPY = 0x00000004,
        };

        VectorImpl(size_t itemSize, uint32_t flags);

        inline const void* arrayImpl() const { return mStorage; }
        void* editArrayImpl();
        virtual inline size_t size() const { return mCount; }
        size_t capacity() const;
        ssize_t setCapacity(size_t size);

        ssize_t insertAt(const void* item, size_t where, size_t numItems = 1);
        const void* itemLocation(size_t index) const;
        void* editItemLocation(size_t index);
    protected:
        void release_storage();
        virtual void do_construct(void* storage, size_t num) const = 0;
        virtual void do_destroy(void* storage, size_t num) const = 0;
        virtual void do_copy(void* dest, const void* from, size_t num) const = 0;
        virtual void do_splat(void* dest, const void* from, size_t num) const = 0;
        virtual void do_move_forward(void* dest, const void* from, size_t num) const = 0;

    private:
        size_t itemSize() const;
        void* _grow(size_t where, size_t amount);
        inline void _do_construct(void* storage, size_t num) const;
        inline void _do_destroy(void* storage, size_t num) const;
        inline void _do_copy(void* dest, const void* from, size_t num) const;
        inline void _do_splat(void* dest, const void* from, size_t num) const;
        inline void _do_move_forward(void* dest, const void* from, size_t num) const;


        void* mStorage;
        size_t mCount;
        const uint32_t mFlags;
        const size_t mItemSize;
    };
} // namespace droid

#endif // DROID_VECTOR_IMPL_H