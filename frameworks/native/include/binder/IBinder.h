#ifndef DROID_IBINDER_H
#define DROID_IBINDER_H

#include "utils/RefBase.h"
#include "utils/String16.h"
#include "utils/Errors.h"
#include "binder/Parcel.h"

// in android, saying that linux/binder.h defines this
// , but it doesn't want to export kernel headers
#ifndef B_PACK_CHARS
#define B_PACK_CHARS(c1, c2, c3, c4) \
	((((c1)<<24)) | (((c2)<<16)) | (((c3)<<8)) | (c4))
#endif

namespace droid {

    class BBinder;
    class BpBinder;
    class IInterface;

    class [[clang::lto_visibility_public]] IBinder
            : public virtual RefBase {
    public:
        enum {
            FIRST_CALL_TRANSACTION = 0x00000001,
            PING_TRANSACTION = B_PACK_CHARS('_', 'P', 'N', 'G'),
        };
        IBinder();

        virtual sp<IInterface> queryLocalInterface(
                const droid::String16& descriptor);

        virtual status_t       transact(uint32_t code
                                        , const Parcel& data
                                        , Parcel* reply
                                        , uint32_t flags = 0) = 0;

        class DeathRecipient : public virtual RefBase {

        };

        virtual BBinder* localBinder();
        virtual BpBinder* remoteBinder();
    };

} // namespace droid
#endif // DROID_IBINDER_H