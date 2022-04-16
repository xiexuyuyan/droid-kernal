#include "utils/String8.h"

#include "log/log.h"
#include <cstring>
#include "utils/String16.h"

#include "SharedBuffer.h"

#undef TAG
#define TAG "String8.cpp"

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

    static char* allocFromUTF16(const char16_t* in, size_t len) {
        if (len == 0) {
            return getEmptyString();
        }

        const ssize_t resultStrLen = utf16_to_utf8_length(in, len) + 1;
        if (resultStrLen < 1) {
            return getEmptyString();
        }

        SharedBuffer* buf = SharedBuffer::alloc(resultStrLen);
        LOGF_ASSERT(buf, "allocFromUTF16: Unable to allocate shared buf");
        if (!buf) {
            return getEmptyString();
        }

        char* resultStr = (char*)buf->data();
        utf16_to_utf8(in, len, resultStr, resultStrLen);
        return resultStr;
    }


    String8::String8() : mString(getEmptyString()){

    }

    String8::String8(const char *o)
        : mString(allocFromUTF8(o, strlen(o))) {
        if (mString == nullptr) {
            mString = getEmptyString();
        }
    }


    String8::String8(const String16& o)
        : mString(allocFromUTF16(o.string(), o.size())) {
    }

    size_t String8::length() const {
        // todo(20220402-112642 why it sub 1)
        // Q: when we do @real_append, we auto append '\0' in the end
        return SharedBuffer::sizeFromData(mString) - 1;
    }

    status_t String8::setTo(const char *other, size_t len) {
        const char* newString = allocFromUTF8(other, len);
        SharedBuffer::bufferFromData(mString)->release();
        mString = newString;
        LOG_ASSERT(mString);
        if (mString)
            return OK;

        mString = getEmptyString();
        return NO_MEMORY;
    }

    status_t String8::append(const char *other) {
        return append(other, strlen(other));
    }

    status_t String8::append(const char *other, size_t otherLen) {
        if (bytes() == 0) {
            return setTo(other, otherLen);
        } else if (otherLen == 0) {
            return OK;
        }

        return real_append(other, otherLen);
    }


    status_t String8::real_append(const char *other, size_t otherLen) {
        const size_t myLen = bytes();
        SharedBuffer* buf=
                SharedBuffer::bufferFromData(
                        mString)->editResize(otherLen + myLen + 1);
        if (buf) {
            char* str = (char*)buf->data();
            mString = str;
            str += myLen;
            memcpy(str, other, otherLen);
            str[otherLen] = '\0';
            return OK;
        }

        return NO_MEMORY;
    }
}