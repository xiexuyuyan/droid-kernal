#include "binder/Status.h"
#include "utils/String8.h"

#include "log/log.h"

#undef TAG
#define TAG "Status.h"

namespace droid {
    namespace binder {

        Status Status::ok() {
            return {};
        }

        String8 Status::toString8() const {
            String8 ret;
            if (mException == EX_NONE) {
                ret.append("No error");
            } else {
                ret.append("Other error");
                LOGF_E(TAG, "toString8: %d!!!", mException);
                // todo(20220402-113625 impl String8::appendFormat)
            }

            return ret;
        }
    } // namespace binder
} // namespace droid

