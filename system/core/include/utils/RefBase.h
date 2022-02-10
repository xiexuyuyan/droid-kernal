#ifndef DROID_REF_BASE_H
#define DROID_REF_BASE_H

#include "log/log.h"

namespace droid {

    template<class T>
    class LightRefBase {
    public:
        inline LightRefBase() : mCount(0) {}

        inline void incStrong(const void *id) const {
            LOG_D("LightRefBase", "incStrong");
            __sync_fetch_and_add(&mCount, 1);
        }

        inline void decStrong(const void *id) const {
            LOG_D("LightRefBase", "decStrong");
            if (__sync_fetch_and_sub(&mCount, 1) == 1) {
                LOG_D("LightRefBase", "delete");
                delete static_cast<const T *>(this);
            }
        }

        inline int32_t getStrongCount() const {
            return mCount;
        }

    protected:
        inline ~LightRefBase() = default;

    private:
        mutable volatile int32_t mCount;
    };

} // namespace droid

#endif //DROID_REF_BASE_H