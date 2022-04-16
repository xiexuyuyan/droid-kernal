#ifndef DROID_IINTERFACE_H
#define DROID_IINTERFACE_H

#include "binder/Binder.h"
#include "log/log.h"

#undef TAG
#define TAG "IInterface.h"

namespace droid {
    // todo(20220324-123218 what means virtual for inheritance?)
    class IInterface : public virtual RefBase {

    };

    template<typename INTERFACE>
    inline sp<INTERFACE> interface_cast(const sp<IBinder>& obj) {
        LOG_D(TAG, "interface_cast: ");
        return INTERFACE::asInterface(obj);
    }

    template<typename INTERFACE>
    class BnInterface : public INTERFACE, public BBinder {
    public:
        sp<IInterface> queryLocalInterface(const String16 &_descriptor);
    };

    template<typename INTERFACE>
    class BpInterface : public INTERFACE, public BpRefBase {
    public:
        explicit BpInterface(const sp<IBinder>& remote);
    };


#define DECLARE_META_INTERFACE(INTERFACE)                   \
public:                                                     \
    static const ::droid::String16 descriptor;              \
    static const ::droid::sp<I##INTERFACE> asInterface(     \
        const ::droid::sp<::droid::IBinder>& obj);          \
public:                                                     \


#define __IINTF_CONCAT(x,y) (x ## y)

#define IMPLEMENT_META_INTERFACE(INTERFACE, NAME)                        \
    const ::droid::StaticString16 I##INTERFACE##_descriptor_static_str16(\
        __IINTF_CONCAT(u,NAME));                                         \
    const ::droid::String16 I##INTERFACE::descriptor(                    \
        I##INTERFACE##_descriptor_static_str16);                         \
    const ::droid::sp<I##INTERFACE> I##INTERFACE::asInterface(           \
        const ::droid::sp<::droid::IBinder>& obj) {                      \
        ::droid::sp<I##INTERFACE> intr;                                  \
        if (obj != nullptr) {                                            \
            intr = static_cast<I##INTERFACE*>(                           \
                obj->queryLocalInterface(                                \
                    I##INTERFACE::descriptor).get());                    \
            if (intr == nullptr) {                                       \
                intr = new Bp##INTERFACE(obj);                           \
            }                                                            \
        }                                                                \
        return intr;                                                     \
    }                                                                    \

    template<typename INTERFACE>
    inline sp<IInterface> BnInterface<INTERFACE>::queryLocalInterface(
            const String16& _descriptor) {
        LOGF_D("TAG", "queryLocalInterface: %s", _descriptor.string());
        if (_descriptor == INTERFACE::descriptor)
            // todo(20220414-191247 override ==)
            return this;
        return nullptr;
    }


    template<typename INTERFACE>
    inline BpInterface<INTERFACE>::BpInterface(
            const sp<IBinder>& remote) : BpRefBase(remote){
        LOG_D(TAG, "BpInterface: constructor");
    }


} // namespace droid

#endif // DROID_IINTERFACE_H