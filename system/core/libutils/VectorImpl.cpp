#include "utils/VectorImpl.h"

#include <string.h>

#include "log/log.h"

#include "SharedBuffer.h"


#undef TAG
#define TAG "VectorImpl.cpp"

namespace droid {
    const size_t kMinVectorCapacity = 4;
    static inline size_t max(size_t a, size_t b) {
        return a > b ? a : b;
    }

    VectorImpl::VectorImpl(size_t itemSize, uint32_t flags)
        : mStorage(nullptr)
        , mCount(0)
        , mFlags(flags)
        , mItemSize(itemSize) {
    }

    void *VectorImpl::editArrayImpl() {
        if (mStorage) {
            const SharedBuffer* sb =
                    SharedBuffer::bufferFromData(mStorage);
            SharedBuffer* editable = sb->attemptEdit();
            if (editable == nullptr) {
                // If we're here, we're not the only owner of the buffer.
                // We must make a copy of it.
                editable = SharedBuffer::alloc(sb->size());
                LOGF_ASSERT(editable == nullptr
                            , "editArrayImpl:%d: we're not the owner"
                            , __LINE__);
                _do_copy(editable->data(), mStorage, mCount);
                release_storage();
                mStorage = editable->data();
            }
        }
        return mStorage;
    }

    size_t VectorImpl::capacity() const {
        if (mStorage) {
            return SharedBuffer::bufferFromData(mStorage)->size() / mItemSize;
        }
        return 0;
    }

    ssize_t VectorImpl::insertAt(
            const void *item, size_t index, size_t numItems) {
        if (index > size()) {
            LOGF_E(TAG, "insertAt: error index = %d", index);
            return BAD_INDEX;
        }
        int preSize = size();
        void* where = _grow(index, numItems);
        size_t whereTransInIndex =
                ((uint8_t*)where - (uint8_t*)mStorage) / mItemSize;
        LOGF_ASSERT(mCount == (preSize + numItems)
                    , "insertAt:%d: After grow, calc a different size"
                      ", preSize = %d, add = %d , finally size = %d"
                      , __LINE__, preSize, numItems, size());
        LOGF_ASSERT(index == whereTransInIndex
                    , "insertAt:%d: After grow, calc a different index"
                      ", we expect = %d, but finally = %d"
                      , __LINE__, index, whereTransInIndex);
        if (where) {
            if (item) {
                _do_splat(where, item, numItems);
            } else {
                _do_construct(where, numItems);
            }
        }
        return where ? index : (ssize_t) NO_MEMORY;
    }

    const void *VectorImpl::itemLocation(size_t index) const {
        LOGF_ASSERT(index < capacity()
                    , "itemLocation:%d: ", __LINE__);
        if (index < capacity()) {
            const void* buffer = arrayImpl();
            if (buffer) {
                return reinterpret_cast<const char*>(buffer)
                            + index * mItemSize;
            }
        }
        return nullptr;
    }

    void *VectorImpl::editItemLocation(size_t index) {
        if (mStorage) {
            const SharedBuffer* sb =
                    SharedBuffer::bufferFromData(mStorage);
            SharedBuffer* editable = sb->attemptEdit();
            if (editable == nullptr) {
                editable = SharedBuffer::alloc(sb->size());
                LOGF_ASSERT(editable, "itemEditLocation: ");
                _do_copy(editable->data(), mStorage, mCount);
                release_storage();
                mStorage = editable->data();
            }
        }
        return mStorage;
    }



    ssize_t VectorImpl::setCapacity(size_t new_capacity) {
        if (new_capacity <= size()) {
            return capacity();
        }

        size_t new_allocation_size = 0;
        bool mAssertTrue = !__builtin_mul_overflow(
                new_capacity, mItemSize, &new_allocation_size);
        LOGF_ASSERT(mAssertTrue, "setCapacity:%d: Mul failed", __LINE__);
        SharedBuffer* sb = SharedBuffer::alloc(new_allocation_size);
        LOGF_ASSERT(sb, "setCapacity:%d: Alloc failed", __LINE__);
        if (sb) {
            void* array = sb->data();
            _do_copy(array, mStorage, size());
            release_storage();
            mStorage = const_cast<void*>(array);
        } else {
            return NO_MEMORY;
        }
        return new_capacity;
    }

    void VectorImpl::release_storage() {
        if (mStorage) {
            const SharedBuffer* sb =
                    SharedBuffer::bufferFromData(mStorage);
            if (sb->release(SharedBuffer::eKeepStorage) == 1) {
                _do_destroy(mStorage, mCount);
                SharedBuffer::dealloc(sb);
            }
        }
    }

    void *VectorImpl::_grow(size_t where, size_t amount) {
        LOGF_ASSERT(where <= mCount
                    , "_grow: where = %d, amount = %d, count = %d"
                    , where, amount, mCount);
        size_t new_size;
        bool mAssertTrue;

        mAssertTrue = !__builtin_add_overflow(mCount, amount, &new_size);
        LOGF_ASSERT(mAssertTrue, "_grow:%d: new_size overflow", __LINE__);

        if (capacity() < new_size) {
            size_t new_capacity = 0;

            mAssertTrue = !__builtin_add_overflow(
                    new_size, (new_size / 2), &new_capacity);
            LOGF_ASSERT(mAssertTrue
                        , "_grow:%d: new_capacity overflow", __LINE__);
            // todo(20220328-102048 in android source:
            //  it makes x = (x + (x/2) + 1)         )
            new_capacity = max(kMinVectorCapacity, new_capacity);

            size_t new_alloc_size = 0;
            mAssertTrue = !__builtin_mul_overflow(
                    new_capacity, mItemSize, &new_alloc_size);
            LOGF_ASSERT(mAssertTrue, "_grow:%d: new_alloc_size overflow"
                        , __LINE__);
            /*
            LOGF_D(TAG
                   , "_grow: from old (capacity = %d size = %d)"
                     ", cause insert amount %d"
                     ", grow to (new_capacity = %d new_size = %d)"
                     , capacity(), size()
                     , amount, new_capacity, new_size);
            */
            if ( (mStorage)
                && (mCount == where)
                && (mFlags & HAS_TRIVIAL_COPY)
                && (mFlags & HAS_TRIVIAL_DTOR)) {
                const SharedBuffer* cur_sb
                    = SharedBuffer::bufferFromData(mStorage);
                SharedBuffer* sb = cur_sb->editResize(new_alloc_size);
                LOGF_ASSERT(sb, "_grow:%d: Resize failed", __LINE__);
                if (sb) {
                    mStorage = sb->data();
                } else {
                    return nullptr;
                }
            } else {
                SharedBuffer* sb = SharedBuffer::alloc(new_alloc_size);
                LOGF_ASSERT(sb
                            , "_grow:%d: Alloc failed, size = %zu"
                            , __LINE__, new_alloc_size);
                if (sb) {
                    void* array = sb->data();
                    if (where != 0) {
                        _do_copy(array, mStorage, where);
                    }
                    // in the root @if, we come here because
                    // @capacity() is smaller than @(mCount + amount)
                    // , and in this function, our target is to grow
                    // , so when @(where == mCount), we are not necessary
                    // to handle anything.
                    if (where != mCount) {
                        const void* from =
                                reinterpret_cast<const uint8_t*>(mStorage)
                                + where * mItemSize;
                        // todo(20220328-193631 why is uint8_t*, we assume sizeof (void* == uint8_t == 8))
                        void* dest = reinterpret_cast<uint8_t*>(array)
                                + (where + amount) * mItemSize;
                        LOGF_ASSERT(mCount > where
                                    , "_grow:%d: Internal Error"
                                    , __LINE__);
                        _do_copy(dest, from, mCount - where);
                    }
                    release_storage();
                    mStorage = const_cast<void*>(array);
                } else {
                    // we make assert above, so we return nullptr directly
                    return nullptr;
                }
            }
        } else {
            void* array = editArrayImpl();
            if (where != mCount) {
                const void* from =
                        reinterpret_cast<const uint8_t*>(array)
                        + where * mItemSize;
                void* to = reinterpret_cast<uint8_t*>(array)
                        + (where + amount) * mItemSize;
                _do_move_forward(to, from, mCount - where);
            }
        }
        mCount = new_size;
        void* free_space = const_cast<void*>(itemLocation(where));
        return free_space;
    }



    size_t VectorImpl::itemSize() const {
        return mItemSize;
    }

    void VectorImpl::_do_construct(void *storage, size_t num) const {
        if (!(mFlags & HAS_TRIVIAL_CTOR)) {
            do_construct(storage, num);
        }
    }

    void VectorImpl::_do_destroy(void *storage, size_t num) const {
        if (!(mFlags & HAS_TRIVIAL_DTOR)) {
            do_destroy(storage, num);
        }
    }

    void VectorImpl::_do_copy(
            void *dest, const void *from, size_t num) const {
        if (!(mFlags & HAS_TRIVIAL_COPY)) {
            do_copy(dest, from, num);
        } else {
            memcpy(dest, from, num * itemSize());
        }
    }


    void VectorImpl::_do_splat(
            void* dest, const void* from, size_t num) const {
        do_splat(dest, from, num);
    }


    void VectorImpl::_do_move_forward(
            void* dest, const void* from, size_t num) const {
        do_move_forward(dest, from, num);
    }


} // namespace droid