#include "utils/String8.h"

#include <cstring>

#include "SharedBuffer.h"

namespace droid {

    static inline char* getEmptyString() {
        static SharedBuffer* gEmptyStringBuf = [] {
            SharedBuffer* buf = SharedBuffer::alloc(1);
            char* str = static_cast<char*>(buf->data());
            *str = 0;
            return buf;
        }();

        // gEmptyStringBuf->acquire();
        // todo(20220326-105625 ref count)
        return static_cast<char*>(gEmptyStringBuf->data());
    }

    static char* allocFromUTF8(const char* in, size_t len) {
        // todo(20220326-110013 to remains that
        //  , there if condition is caused by machine or system
        //  , we return a empty result to user
        //  , but if  caused by users, we return nullptr)
        if (len > 0) {
            if (len == SIZE_MAX)
                return nullptr;

            SharedBuffer* buf = SharedBuffer::alloc(len+1);
            LOGF_ASSERT(buf, "Unable to allocate shared buffer");
            if (buf) {
                char* str =(char*)buf->data();
                memcpy(str, in, len);
                str[len] = 0;
                return str;
            }
            return nullptr;
        }
        return getEmptyString();
    }

    String8::String8(const char *o)
        : mString(allocFromUTF8(o, strlen(o))) {
        if (mString == nullptr) {
            mString = getEmptyString();
        }
    }
}