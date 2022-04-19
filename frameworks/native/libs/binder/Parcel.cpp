#include <limits>
#include <cstring>
#include "binder/Parcel.h"

#include "utils/Errors.h"
#include "log/log.h"

#undef TAG
#define TAG "Parcel.cpp"

#define LOG_DEBUG_PARCEL_ALLOC
#ifdef LOG_DEBUG_PARCEL_ALLOC
#define LOGF_ALLOC(TAG, FORMAT, ...) LOGF_V((TAG"_ALLOC"), FORMAT, ##__VA_ARGS__)
#else
#define LOGF_ALLOC(...)
#endif // LOG_DEBUG_PARCEL_ALLOC

// this macro should never be used at runtime
#define PAD_SIZE_UNSAFE(s) (((s) + 3) & ~3)

static size_t pad_size(size_t s) {
    if (s > (std::numeric_limits<size_t>::max() - 3)) {
        LOGF_E(TAG, "pad_size: pad size is too big %zu", s);
    }
    return PAD_SIZE_UNSAFE(s);
}

static size_t max(size_t d1, size_t d2) {
    return (d1 > d2) ? d1 : d2;
}

namespace droid {

    static pthread_mutex_t gParcelGlobalAllocSizeLock =
            PTHREAD_MUTEX_INITIALIZER;
    static size_t gParcelGlobalAllocSize = 0;
    static size_t gParcelGlobalAllocCount = 0;

    Parcel::Parcel() {
        initState();
    }

    const uint8_t* Parcel::data() const{
        return mData;
    }

    size_t Parcel::dataSize() const {
        return max(mDataSize, mDataPos);
    }

    size_t Parcel::dataAvail() const {
        size_t result = dataSize() - dataPosition();
        if (result > INT32_MAX) {
            // see: @finishWrite
            LOGF_E(TAG, "dataAvail: result too big: %zu", result);
        }
        return result;
    }

    size_t Parcel::dataPosition() const {
        return mDataPos;
    }

    size_t Parcel::dataCapacity() const {
        return mDataCapacity;
    }

    status_t Parcel::errorCheck() const {
        return mError;
    }

    void Parcel::setError(status_t err) {
        mError = err;
    }

    status_t Parcel::write(const void* data, size_t len) {
        if (len > INT32_MAX) {
            // see: @finishWrite
            return BAD_VALUE;
        }

        void* const d = writeInplace(len);
        if (d) {
            memcpy(d, data, len);
            return NO_ERROR;
        }

        return mError;
    }


    void *Parcel::writeInplace(size_t len) {
        if (len > INT32_MAX) return nullptr;

        const size_t padded = pad_size(len);

        if (mDataPos + padded < mDataPos) return nullptr;

        if ((mDataPos + padded) <= mDataCapacity) {
restart_write:
            uint8_t* const data = mData + mDataPos;

            if (padded != len) {
#if BYTE_ORDER == BIG_ENDIAN
                static const uint32_t mask[4] = {
                        0x00000000, 0xFFFFFF00, 0xFFFF0000, 0xFF000000
                };
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
                static const uint32_t mask[4] = {
                        0x00000000, 0x00FFFFFF, 0x0000FFFF, 0x000000FF
                };
#endif
                *reinterpret_cast<uint32_t*>(data + padded - 4) &= mask[padded - len];
            }
            finishWrite(padded);
            return data;
        }

        status_t err = growData(padded);
        if (err == NO_ERROR) {
            goto restart_write;
        }
        return nullptr;
    }

    status_t Parcel::writeInt32(int32_t val) {
        return writeAligned(val);
    }

    uintptr_t Parcel::ipcData() const {
        return reinterpret_cast<uintptr_t>(mData);
    }

    size_t Parcel::ipcDataSize() const {
        return max(mDataSize, mDataPos);
    }

    uintptr_t Parcel::ipcObjects() const {
        return reinterpret_cast<uintptr_t>(mObjects);
    }

    size_t Parcel::ipcObjectsCount() const {
        return mObjectsSize;
    }


    status_t Parcel::finishWrite(size_t len) {
        if (len > INT32_MAX) {
            // don't accept size_t values which may have come from
            // an inadvertent conversion from a negative int.
            return BAD_VALUE;
        }

        mDataPos += len;
        LOGF_ALLOC(TAG, "finishWrite: Setting data pos of %p to %zu"
        , this, mDataPos);
        if (mDataPos > mDataSize) {
            mDataSize = mDataPos;
            LOGF_ALLOC(TAG, "finishWrite: Setting data size of %p to %zu"
            , this, mDataPos);
        }

        return NO_ERROR;
    }


    status_t Parcel::growData(size_t len) {
        if (len > INT32_MAX) {
            // see: @finishWrite
            return BAD_VALUE;
        }

        if (len > (SIZE_MAX - mDataSize))
            return NO_MEMORY;
        if (mDataSize + len > SIZE_MAX / 3)
            // todo(20220418-140855 why the @mDataSize is limited to 1/3)
            return NO_MEMORY;
        size_t newSize = ((mDataSize + len) * 3) / 2;
        return (newSize <= mDataSize)
               ? NO_MEMORY
               : continueWrite(newSize);
    }


    status_t Parcel::continueWrite(size_t desired) {
        if (desired > INT32_MAX) {
            // see: @finishWrite
            return BAD_VALUE;
        }

        // If shrinking first adjust for any objects that
        // appear after the new data size.
        // todo(20220418-153439
        //  to delete: think about how does it related with @objectsSize)
        size_t objectsSize = mObjectsSize;
        if (desired < mDataSize) {
            if (desired == 0) {
                objectsSize = 0;
            } else {
                while (objectsSize > 0) {
                    if (mObjects[objectsSize - 1] < desired) {
                        break;
                    }
                    objectsSize--;
                }
            }
        }

        if (mOwner) {
            // todo(20220418-155043 complete when @ipcSetDataReference
            //  is called, and it sets @mOwner)
            LOG_E(TAG, "continueWrite: todo-err");
        } else if (mData) {
            if (objectsSize < mObjectsSize) {
                // todo(20220418-155258 it comes when we reduce
                //  the size of @mObjects)
                LOG_E(TAG, "continueWrite: todo-err");
            }

            if (desired > mDataCapacity) {
                /*uint8_t*/auto* data = (uint8_t*)realloc(mData, desired);
                if (data) {
                    LOGF_ALLOC(TAG, "continueWrite: "
                                    "%p continue from %zu to %zu capacity"
                    , this, mDataCapacity, desired);
                    pthread_mutex_lock(&gParcelGlobalAllocSizeLock);
                    gParcelGlobalAllocSize += desired;
                    gParcelGlobalAllocSize -= mDataCapacity;
                    pthread_mutex_unlock(&gParcelGlobalAllocSizeLock);
                    mData = data;
                    mDataCapacity = desired;
                    // todo(20220418-160343 to comprehend
                    //  @gParcelGlobalAllocSize and @mDataCapacity)
                } else {
                    mError = NO_MEMORY;
                    return NO_MEMORY;
                }
            } else {
                if (mDataSize > desired) {
                    mDataSize = desired;
                    LOGF_ALLOC(TAG, "continueWrite: "
                                    "Setting data size of %p to %zu"
                    , this, mDataSize);
                }
                if (mDataPos > desired) {
                    mDataPos = desired;
                    LOGF_ALLOC(TAG, "continueWrite: "
                                    "Setting data pos of %p to %zu"
                    , this, mDataPos);
                }
            }
        } else {
            // this is the first data. Easy!
            /*uint8_t*/auto* data = (uint8_t*)malloc(desired);
            if (!data) {
                mError = NO_MEMORY;
                return NO_MEMORY;
            }

            if (!(mDataCapacity == 0
                  && mObjects == nullptr
                  && mObjectsCapacity == 0)) {
                LOGF_E(TAG, "continueWrite: "
                            "mDataCapacity(%zu), mObjects(%p)"
                            ", mObjectsSize(%zu)"
                , mDataCapacity, mObjects, mObjectsSize);
            }

            LOGF_ALLOC(TAG, "continueWrite: %p: allocating with %zu capacity"
            , this, desired);
            pthread_mutex_lock(&gParcelGlobalAllocSizeLock);
            gParcelGlobalAllocSize += desired;
            gParcelGlobalAllocCount++;
            pthread_mutex_unlock(&gParcelGlobalAllocSizeLock);

            mData = data;
            mDataSize = mDataPos = 0;
            LOGF_ALLOC(TAG, "continueWrite: Setting data size of %p to %zu"
            , this, mDataSize);
            LOGF_ALLOC(TAG, "continueWrite: Setting data pos of %p to %zu"
            , this, mDataSize);
            mDataCapacity = desired;
        }

        return NO_ERROR;
    }

    void Parcel::initState() {
        mError = NO_ERROR;
        mData = nullptr;
        mDataSize = 0;
        mDataCapacity = 0;
        mDataPos = 0;
        LOGF_ALLOC(TAG, "initState: Setting data size of %p to %zu"
        , this, mDataSize);
        LOGF_ALLOC(TAG, "initState: Setting data pos of %p to %zu"
        , this, mDataPos);
        mObjects = nullptr;
        mObjectsSize = 0;
        mObjectsCapacity = 0;

        mOwner = nullptr;
    }

    template<class T>
    status_t Parcel::writeAligned(T val) {
        // todo(20220418-123839 COMPILE_TIME_ASSERT_FUNCTION_SCOPE)
        static_assert(PAD_SIZE_UNSAFE(sizeof(T)) == sizeof(T)
                , "COMPILE_TIME_ASSERT_FUNCTION_SCOPE");

        if ((mDataPos + sizeof(val)) <= mDataCapacity) {
restart_write:
            *reinterpret_cast<T*>(mData + mDataPos) = val;
            return finishWrite(sizeof(val));
        }

        status_t err = growData(sizeof(val));
        if (err == NO_ERROR) {
            goto restart_write;
        }
        return err;
    }


} // namespace droid