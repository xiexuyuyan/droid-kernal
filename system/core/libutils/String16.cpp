#include "utils/String16.h"
#include "utils/Unicode.h"
#include "SharedBuffer.h"

namespace droid {
    static const StaticString16 emptyString(u"");
    static inline char16_t* getEmptyString() {
        return const_cast<char16_t*>(emptyString.string());
    }

    void *String16::alloc(size_t size) {
        SharedBuffer* buf = SharedBuffer::alloc(size);
        buf->mClientMetadata = kIsSharedBufferAllocated;
        return buf;
    }

    char16_t *String16::allocFromUTF8(const char *u8str, size_t u8len) {
        if (u8len == 0) {
            return getEmptyString();
        }

        const uint8_t* u8cur = (const uint8_t*) u8str;

        const ssize_t u16len = utf8_to_utf16_length(u8cur, u8len);
        if (u16len < 0) {
            return getEmptyString();
        }

        SharedBuffer* buf = static_cast<SharedBuffer*>(alloc(sizeof(char16_t) * (u16len + 1)));
        if (buf) {
            u8cur = (const uint8_t*) u8str;
            char16_t* u16str = (char16_t *)buf->data();

            utf8_to_utf16(u8cur, u8len, u16str, ((size_t) u16len) + 1);

            return u16str;
        }

        return getEmptyString();
    }

    String16::String16(const char *o)
        : mString(allocFromUTF8(o, strlen(o))){}

    size_t String16::size() const {
        if (isStaticString()) {
            return staticStringSize();
        } else {
            return SharedBuffer::
                sizeFromData(mString) / sizeof(char16_t)- 1;
        }
    }

    bool String16::isStaticString() const {
        static_assert(
                sizeof(SharedBuffer) -
                offsetof(SharedBuffer, mClientMetadata) == 4);
        const uint32_t* p = reinterpret_cast<const uint32_t*>(mString);
        // In static case: p = @Static.data
        // , so *(p - 1) = size, it is no more than 0x80000000
        // In non-static case: p = @SharedBuffer.data()
        // , so *(p - 1) = the last member of SharedBuffer
        // , meaning it comes as we assert above, equals @mClientMetadata
        return (*(p - 1) & kIsSharedBufferAllocated) == 0;
    }

    size_t String16::staticStringSize() const {
        static_assert(
                sizeof(SharedBuffer) -
                offsetof(SharedBuffer, mClientMetadata) == 4);
        const uint32_t* p = reinterpret_cast<const uint32_t*>(mString);
        // todo(20220415-145623 the size include mClientMetadata ??)
        return static_cast<size_t>(*(p - 1));
    }

} // namespace droid
