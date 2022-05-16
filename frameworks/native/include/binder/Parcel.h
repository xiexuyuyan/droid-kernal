#ifndef DROID_PARCEL_H
#define DROID_PARCEL_H

#include "utils/Errors.h"
#include "utils/RefBase.h"

#include <linux/android/binder.h>

namespace droid {
    class Parcel {
        friend class IPCThreadState;
    public:
                            Parcel();

        const uint8_t*      data() const;
        size_t              dataSize() const;
        size_t              dataAvail() const;
        size_t              dataPosition() const;
        size_t              dataCapacity() const;

        status_t            setDataSize(size_t size);
        void                setDataPosition(size_t size) const;

        status_t            errorCheck() const;
        void                setError(status_t err);
        status_t            write(const void* data, size_t len);
        void*               writeInplace(size_t len);
        status_t            writeInt32(int32_t val);

    private:
        typedef void        (*release_func)(
                                Parcel* parcel
                                , const uint8_t* data, size_t dataSize
                                , const binder_size_t* objects
                                    , size_t objectsSize
                                , void* cookie);
        uintptr_t           ipcData() const;
        size_t              ipcDataSize() const;
        uintptr_t           ipcObjects() const;
        size_t              ipcObjectsCount() const;
        void                ipcSetDataReference(const uint8_t* data
                                                , size_t dataSize
                                                , const binder_size_t* objects
                                                , size_t objectsCount
                                                , release_func releaseFunc
                                                , void* releaseCookie);
        status_t            finishWrite(size_t len);
        status_t            growData(size_t len);
        status_t            continueWrite(size_t len);
        void                releaseObjects();
        void                acquireObjects();
        void                freeData();
        void                freeDataNoInit();
        void                initState();
        void                scanForFds() const;
        template<class T>
        status_t            writeAligned(T val);

        status_t            mError;
        uint8_t*            mData;
        size_t              mDataSize;
        size_t              mDataCapacity;
        mutable size_t      mDataPos;// cursor
        binder_size_t*      mObjects;
        size_t              mObjectsSize;
        size_t              mObjectsCapacity;
        mutable size_t      mNextObjectHint;
        mutable bool        mObjectsSorted;

        mutable bool        mFdsKnown;
        mutable bool        mHasFds;

        release_func        mOwner;
        void*               mOwnerCookie;

    private:
        size_t mOpenAshmemSize;
    };
} // namespace droid

#endif //DROID_PARCEL_H
