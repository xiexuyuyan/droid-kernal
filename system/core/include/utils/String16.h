#ifndef DROID_STRING16_H
#define DROID_STRING16_H

#include <cstring>
#include <cstdint>
#include "Unicode.h"

#include "log/log.h"

#undef TAG
#define TAG "String16.h"

namespace droid {
    template<size_t N>
    class StaticString16;

    class String16 {
    public:
        size_t           size() const;
        explicit         String16(const char* o);
        inline const     char16_t * string() const;
        inline bool      operator==(const String16& other) const;

               bool      isStaticString() const;
    private:
        static constexpr uint32_t kIsSharedBufferAllocated = 0x80000000;
        static void*     alloc(size_t size);
        static char16_t* allocFromUTF8(const char* u8str, size_t u8len);
               size_t    staticStringSize() const;
        const  char16_t* mString;
    protected:
        template<size_t N>
        struct StaticData {
            static_assert(N - 1 < kIsSharedBufferAllocated, "StaticString16 too long!");
            constexpr StaticData() : size(N - 1), data{0} {}
            const uint32_t size;
            char16_t data[N];

            constexpr StaticData(const StaticData<N>&) = default;
        };

        template <size_t N>
        static constexpr const StaticData<N> makeStaticData(const char16_t (&s)[N]) {
            StaticData<N> r;
            for (size_t i = 0; i < N - 1; ++i) {
                r.data[i] = s[i];
            }
            return r;
        }

        template<size_t N>
        explicit constexpr String16(const StaticData<N>& s) : mString(s.data) {}
    };


    template <size_t N>
    class StaticString16 : public String16 {
    public:
        constexpr StaticString16(const char16_t (&s)[N])
            : String16(mData), mData(makeStaticData(s)) {
        }
    private:
        const StaticData<N> mData;
    };



    const char16_t *String16::string() const {
        return mString;
    }

    bool String16::operator==(const String16 &other) const {
        return strzcmp16(mString, size()
                         , other.mString, other.size()) == 0;
    }
} // namespace droid

#endif // DROID_STRING16_H