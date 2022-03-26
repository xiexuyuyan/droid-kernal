#ifndef DROID_STRING8_H
#define DROID_STRING8_H

namespace droid {
    class String8 {
    public:
        explicit String8(const char* o);

        inline const char* c_str() const;
        inline const char* string() const;
    private:
        const char* mString;
    };

    inline const char* String8::c_str() const {
        return mString;
    }
    inline const char* String8::string() const {
        return mString;
    }

} // namespace droid


#endif // DROID_STRING8_H