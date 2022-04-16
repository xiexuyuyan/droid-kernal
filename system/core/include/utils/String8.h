#ifndef DROID_STRING8_H
#define DROID_STRING8_H

#include "utils/Errors.h"

namespace droid {
    class String16;

    class String8 {
    public:
                               String8();
        explicit               String8(const char* o);

        explicit               String8(const String16& o);

        inline const char*     c_str() const;
        inline const char*     string() const;
        inline       size_t    bytes() const;
                     size_t    length() const;
                     status_t  setTo(const char* other, size_t numChars);
                     status_t  append(const char* other);
                     status_t  append(const char* other, size_t numChars);
    private:
               const char*    mString;
                     status_t real_append(const char* other, size_t numChars);
    };

    inline const char* String8::c_str() const {
        return mString;
    }
    inline const char* String8::string() const {
        return mString;
    }

    size_t String8::bytes() const {
        return length();
    }

} // namespace droid


#endif // DROID_STRING8_H