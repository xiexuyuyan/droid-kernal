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
            };
            static Status  ok();
                   bool    isOk() const { return mException == EX_NONE; }
                   String8 toString8() const;
        private:
            int32_t mException = EX_NONE;
        };


    } // namespace binder
} // namespace droid


#endif
