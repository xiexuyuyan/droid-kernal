#ifndef DROID_BINDER_STATUS_H
#define DROID_BINDER_STATUS_H

#include <cstdint>
#include "log/log.h"
#include "utils/String8.h"

#undef TAG
#define TAG "Status.h"

namespace droid {
    namespace binder {
        class Status final {
        public:
                   Status() = default;
                   ~Status() = default;

            enum Exception {
                EX_NONE = 0,
                EX_ILLEGAL_ARGUMENT = -3,

                EX_TRANSACTION_FAILED = -129,
            };
            static Status  ok();
            static Status  fromExceptionCode(int32_t exceptionCode);
                   bool    isOk() const { return mException == EX_NONE; }
                   String8 toString8() const;
        private:
            Status(int32_t exceptionCode, int32_t errorCode);
            int32_t mException = EX_NONE;
            int32_t mErrorCode = 0;
        };


    } // namespace binder
} // namespace droid


#endif
