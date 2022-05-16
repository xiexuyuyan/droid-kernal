#include <limits>
#include <cstring>
#include "binder/Parcel.h"

#include "utils/Errors.h"
#include "log/log.h"
#include "binder/ProcessState.h"

#undef TAG
#define TAG "Parcel.cpp"

#define LOG_DEBUG_PARCEL_ALLOC
#ifdef LOG_DEBUG_PARCEL_ALLOC
#define LOGF_ALLOC(TAG, FORMAT, ...) LOGF_V((TAG"_ALLOC"), FORMAT, ##__VA_ARGS__)
#else
#define LOGF_ALLOC(...)
#endif // LOG_DEBUG_PARCEL_ALLOC

#define LOG_DEBUG_PARCEL_REFS
#ifdef LOG_DEBUG_PARCEL_REFS
#define LOGF_REFS(TAG, FORMAT, ...) LOGF_V((TAG"_REFS"), FORMAT, ##__VA_ARGS__)
#else
#define LOGF_REFS(...)
#endif // LOG_DEBUG_PARCEL_REFS

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

    static void acquire_object(
            const sp<ProcessState>& proc
            , const flat_binder_object& obj
            , const void* who
            , size_t* outAshmemSize) {
        switch (obj.hdr.type) {
            case BINDER_TYPE_BINDER:
                if (obj.binder) {
                    LOGF_REFS(TAG, "acquire_object %p: "
                                   "acquiring reference on local %p"
                              , who, obj.cookie);
                    reinterpret_cast<IBinder*>(obj.cookie)->incStrong(who);
                }
                return;

            case BINDER_TYPE_HANDLE: {
                const sp<IBinder> b =
                        proc->getStrongProxyForHandle(obj.handle);
                if (b != nullptr) {
                    LOGF_REFS(TAG, "acquire_object %p: "
                                   "acquiring reference on remote %p"
                              , who, b.get());
                    b->decStrong(who);
                }
                return;
            }

            case BINDER_TYPE_FD: {
                LOG_E(TAG, "acquire_object: todo-err");
                // todo(20220511-183433 to complete binder flat is a fd)
                return;
            }
        }

        LOGF_E(TAG, "acquire_object: Invalid object type 0x%08x"
               , obj.hdr.type);
    }

    static void release_object(
            const sp<ProcessState>& proc
            , const flat_binder_object& obj
            , const void* who
            , size_t* outAshmemSize) {
        switch (obj.hdr.type) {
            case BINDER_TYPE_BINDER:
                if (obj.binder) {
                    LOGF_REFS(TAG, "release_object %p: "
                                   "releasing reference on local %p"
                          , who, obj.cookie);
                    reinterpret_cast<IBinder*>(obj.cookie)->decStrong(who);
                }
                return;

            case BINDER_TYPE_HANDLE: {
                const sp<IBinder> b =
                        proc->getStrongProxyForHandle(obj.handle);
                if (b != nullptr) {
                    LOGF_REFS(TAG, "release_object %p: "
                                   "releasing reference on remote %p"
                              , who, b.get());
                    b->decStrong(who);
                }
                return;
            }

            case BINDER_TYPE_FD: {
                LOG_E(TAG, "release_object: todo-err");
                // todo(20220511-183433 to complete binder flat is a fd)
                return;
            }
        }

        LOGF_E(TAG, "release_object: Invalid object type 0x%08x"
               , obj.hdr.type);
    }

    Parcel::Parcel() {
        LOGF_ALLOC(TAG, "Parcel %p: constructing", this);
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

    status_t Parcel::setDataSize(size_t size) {
        if (size > INT32_MAX) {
            return BAD_VALUE;
        }

        status_t err;
        err = continueWrite(size);
        if (err == NO_ERROR) {
            mDataSize = size;
            LOGF_V(TAG, "setDataSize: Setting data size of %p to %zu"
                   , this, mDataSize);
        }

        return err;
    }

    void Parcel::setDataPosition(size_t pos) const {
        if (pos > INT32_MAX) {
            LOGF_E(TAG, "setDataPosition: pos too big: %zu", pos);
        }

        mDataPos = pos;
        mNextObjectHint = 0;
        mObjectsSorted = false;
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

    void Parcel::ipcSetDataReference(
            const uint8_t *data
            , size_t dataSize
            , const binder_size_t *objects
            , size_t objectsCount
            , Parcel::release_func releaseFunc
            , void *releaseCookie) {
        binder_size_t minOffset = 0;
        freeDataNoInit();
        mError = NO_ERROR;
        mData = const_cast<uint8_t*>(data);
        mDataSize = mDataCapacity = dataSize;
        LOGF_REFS(TAG,
            "ipcSetDataReference: Setting data pos of %p to %zu"
            , this, mDataPos);
        mObjects = const_cast<binder_size_t*>(objects);
        mObjectsSize = mObjectsCapacity = objectsCount;
        mNextObjectHint = 0;
        mObjectsSorted = false;
        mOwner = releaseFunc;
        // todo(20220512-102424 what means @mOwnerCookie)
        mOwnerCookie = releaseCookie;
        for (size_t i = 0; i < mObjectsSize; i++) {
            binder_size_t offset = mObjects[i];
            if (offset < minOffset) {
                LOGF_E(TAG, "ipcSetDataReference"
                    ": bad object offset %I64u < %%I64u\n"
                    , (uint64_t)offset, uint64_t(minOffset));
                mObjectsSize = 0;
                break;
            }
            const flat_binder_object* flat =
                    reinterpret_cast<const flat_binder_object*>(
                            mData + offset);
            uint32_t type = flat->hdr.type;
            if (!(type == BINDER_TYPE_BINDER
                || type == BINDER_TYPE_HANDLE
                || type == BINDER_TYPE_FD)) {
                LOGF_E(TAG, "ipcSetDataReference: "
                    "unsupported type object %I32u at offset %I64u"
                    , type, (uint64_t)offset);
                releaseObjects();
                mObjectsSize = 0;
                break;
            }
            minOffset = offset + sizeof(flat_binder_object);
        }
        scanForFds();
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
            if (desired == 0) {
                freeData();
                return NO_ERROR;
            }

            uint8_t* data = (uint8_t*)malloc(desired);
            if (!data) {
                mError = NO_MEMORY;
                return NO_MEMORY;
            }
            binder_size_t* objects = nullptr;

            if (objectsSize) {
                objects = (binder_size_t*)calloc(
                        objectsSize, sizeof(binder_size_t));
                if (!objects) {
                    free(data);

                    mError = NO_MEMORY;
                    return NO_MEMORY;
                }
                size_t oldObjectsSize = mObjectsSize;
                mObjectsSize = objectsSize;
                acquireObjects();
                mObjectsSize = oldObjectsSize;
            }

            if (mData) {
                memcpy(data, mData
                       , mDataSize < desired ? mDataSize : desired);
            }
            if (objects && mObjects) {
                memcpy(objects, mObjects
                       , objectsSize * sizeof(binder_size_t));
            }

            mOwner(this, mData, mDataSize
                   , mObjects, mObjectsSize, mOwnerCookie);
            mOwner = nullptr;

            LOGF_ALLOC(TAG, "%p continueWrite: "
                            "taking ownership of %zu capacity"
                            , this, desired);
            pthread_mutex_lock(&gParcelGlobalAllocSizeLock);
            gParcelGlobalAllocSize += desired;
            gParcelGlobalAllocCount++;
            pthread_mutex_unlock(&gParcelGlobalAllocSizeLock);

            mData = data;
            mObjects = objects;
            mDataSize = (mDataSize < desired) ? mDataSize : desired;
            LOGF_V(TAG, "continueWrite: Setting data size of %p to %zu"
                   , this, mDataSize);
            mDataCapacity = desired;
            mObjectsSize = mObjectsCapacity = objectsSize;
            mNextObjectHint = 0;
            mObjectsSorted = false;

        } else if (mData) {
            if (objectsSize < mObjectsSize) {
                const sp<ProcessState> proc(ProcessState::self());
                for (size_t i = objectsSize; i < mObjectsSize; i++) {
                    const flat_binder_object* flat =
                            reinterpret_cast<flat_binder_object*>(mData + mObjects[i]);
                    if (flat->hdr.type == BINDER_TYPE_FD) {
                        mFdsKnown = false;
                    }
                    release_object(proc, *flat, this, &mOpenAshmemSize);
                }

                if (objectsSize == 0) {
                    free(mObjects);
                    mObjects = nullptr;
                    mObjectsCapacity = 0;
                } else {
                    binder_size_t* objects = (binder_size_t*)realloc(
                            mObjects, objectsSize * sizeof(binder_size_t));
                    if (objects) {
                        mObjects = objects;
                        mObjectsCapacity = objectsSize;
                    }
                }

                mObjectsSize = objectsSize;
                mNextObjectHint = 0;
                mObjectsSorted = false;
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

            LOGF_ALLOC(TAG, "continueWrite: %p allocating with %zu capacity"
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

    void Parcel::releaseObjects() {
        size_t i = mObjectsSize;
        if (i == 0) {
            return;
        }

        sp<ProcessState> proc(ProcessState::self());
        uint8_t* const data = mData;
        binder_size_t* const objects = mObjects;
        while (i > 0) {
            i--;
            const flat_binder_object* flat =
                    reinterpret_cast<flat_binder_object*>(data + objects[i]);
            release_object(proc, *flat, this/*who*/, &mOpenAshmemSize);
        }
    }

    void Parcel::acquireObjects() {
        size_t i = mObjectsSize;
        if (i == 0) {
            return;
        }
        const sp<ProcessState> proc(ProcessState::self());
        uint8_t* const data = mData;
        binder_size_t* const objects = mObjects;
        while (i > 0) {
            i--;
            const flat_binder_object* flat =
                    reinterpret_cast<flat_binder_object*>(
                            data + objects[i]);
            acquire_object(proc, *flat, this, &mOpenAshmemSize);
        }
    }

    void Parcel::freeData() {
        freeDataNoInit();
        initState();
    }

    void Parcel::freeDataNoInit() {
        if (mOwner) {
            mOwner(this, mData, mDataSize
                   , mObjects, mObjectsSize, mOwnerCookie);
        } else {
            LOGF_ALLOC(TAG
                , "freeDataNoInit %p: freeing allocated data", this);
            releaseObjects();
            if (mData) {
                LOGF_ALLOC(TAG,
                    "freeDataNoInit %p: freeing with %zu capacity"
                    , this, mDataCapacity);
                pthread_mutex_lock(&gParcelGlobalAllocSizeLock);
                if (mDataCapacity <= gParcelGlobalAllocSize) {
                    //  when control by mOwner
                    //  , it only increase by @continueWrite(...)
                    //  , so when we need decrease
                    //  , just subtract @mDataCapacity we set in @continueWrite
                    //  which equals desired number)
                    gParcelGlobalAllocSize = gParcelGlobalAllocSize - mDataCapacity;
                } else {
                    gParcelGlobalAllocSize = 0;
                }
                pthread_mutex_unlock(&gParcelGlobalAllocSizeLock);
                free(mData);
            }
            if (mObjects)
                free(mObjects);
        }
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
        mNextObjectHint = 0;
        mObjectsSorted = false;
        mHasFds = false;
        mFdsKnown = false;
        mOwner = nullptr;
        mOpenAshmemSize = 0;
    }

    void Parcel::scanForFds() const {
        bool hasFds = false;
        for (size_t i = 0; i < mObjectsSize; i++) {
            const flat_binder_object* flat =
                    reinterpret_cast<const flat_binder_object*>(
                            mData + mObjects[i]);
            if (flat->hdr.type == BINDER_TYPE_FD) {
                hasFds = true;
                break;
            }
        }
        mHasFds = hasFds;
        mFdsKnown = true;
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