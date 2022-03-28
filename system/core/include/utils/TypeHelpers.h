#ifndef DROID_TYPE_HELPERS_H
#define DROID_TYPE_HELPERS_H

#include <string.h>

namespace droid {

    template <typename T> struct trait_pointer      { enum { value = false }; };
    template <typename T> struct trait_pointer<T*>  { enum { value = true  }; };
    template <typename T> struct trait_trivial_ctor { enum { value = false }; };
    template <typename T> struct trait_trivial_dtor { enum { value = false }; };
    template <typename T> struct trait_trivial_copy { enum { value = false }; };
    template <typename T> struct trait_trivial_move { enum { value = false }; };

    template <typename TYPE>
    struct traits {
        enum {
            is_pointer       = trait_pointer<TYPE>::value,
            has_trivial_ctor = is_pointer || trait_trivial_ctor<TYPE>::value,
            has_trivial_dtor = is_pointer || trait_trivial_dtor<TYPE>::value,
            has_trivial_copy = is_pointer || trait_trivial_copy<TYPE>::value,
            has_trivial_move = is_pointer || trait_trivial_move<TYPE>::value,
        };
    };

    template <typename TYPE> inline
    void destroy_type(TYPE* p, size_t n) {
        if (!traits<TYPE>::has_trivial_dtor) {
            while (n > 0) {
                n--;
                p->~TYPE();
                p++;
            }
        }
    }


    template <typename TYPE>
    typename std::enable_if<traits<TYPE>::has_trivial_copy>::type
    inline
    copy_type(TYPE* d, const void* s, size_t n) {
        memcpy(d, s, n);
    }

    template <typename TYPE>
    typename std::enable_if<!traits<TYPE>::has_trivial_copy>::type
    inline
    copy_type(TYPE* d, const TYPE* s, size_t n) {
        while (n > 0) {
            n--;
            // todo(20220328-184209 what is n in this case? maybe item nums.)
            new(d) TYPE(*s);
            d++, s++;
        }
    }

    template <typename TYPE>
    struct use_trivial_move : public std::integral_constant<bool
        , (traits<TYPE>::has_trivial_dtor
            && traits<TYPE>::has_trivial_ctor)
            || traits<TYPE>::has_trivial_move
        > {};

    template <typename TYPE>
    typename std::enable_if<use_trivial_move<TYPE>::value>::type
    inline
    move_forward_type(TYPE* d, const TYPE* s, size_t n = 1) {
        memmove(d, s, n * sizeof(TYPE));
    }
    template <typename TYPE>
    typename std::enable_if<!use_trivial_move<TYPE>::value>::type
    inline
    move_forward_type(TYPE* d, const TYPE* s, size_t n = 1) {
        d += n;
        s += n;
        while (n > 0) {
            n--;
            --d, --s;
            if (!traits<TYPE>::has_trivial_copy) {
                new(d) TYPE(*s);
            } else {
                *d = *s;
            }
            if (!traits<TYPE>::has_trivial_dtor) {
                s->~TYPE();
            }
        }
    }


} // namespace droid

#endif // DROID_TYPE_HELPERS_H