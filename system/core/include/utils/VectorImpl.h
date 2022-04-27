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

        inline  const void*     arrayImpl() const { return mStorage; }
                void*           editArrayImpl();
        virtual inline size_t   size() const { return mCount; }
                inline bool     isEmpty() const { return mCount == 0; };
                size_t          capacity() const;
                ssize_t         setCapacity(size_t size);

                ssize_t         insertAt(const void* item, size_t where, size_t numItems = 1);
                void            push();
                void            push(const void* item);
                ssize_t         removeItemsAt(size_t index, size_t count = 1);
                void            clear();
                const void*     itemLocation(size_t index) const;
                void*           editItemLocation(size_t index);
    protected:
                size_t          itemSize() const;
                void            release_storage();
        virtual void do_construct(void* storage, size_t num) const = 0;
        virtual void do_destroy(void* storage, size_t num) const = 0;
        virtual void do_copy(void* dest, const void* from, size_t num) const = 0;
        virtual void do_splat(void* dest, const void* from, size_t num) const = 0;
        virtual void do_move_forward(void* dest, const void* from, size_t num) const = 0;

    private:
        void* _grow(size_t where, size_t amount);
        void  _shrink(size_t where, size_t amount);
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

    class SortedVectorImpl : public VectorImpl {
    public:
                                SortedVectorImpl(size_t itemSize, uint32_t flags);

                ssize_t         indexOf(const void* item) const;
    protected:
        virtual int             do_compare(const void* lhs, const void* rhs) const = 0;
    private:
                ssize_t         _indexOrderOf(
                        const void* item, size_t* order = nullptr) const;
    };

} // namespace droid

#endif // DROID_VECTOR_IMPL_H