#ifndef DROID_TYPE_HELPERS_H
#define DROID_TYPE_HELPERS_H

#include <string.h>

#include "log/log.h"

#undef TAG
#define TAG "TypeHelpers.h"

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

    template<typename T, typename U>
    struct aggregate_traits {
        enum {
            is_pointer       = false,
            has_trivial_ctor =
                    traits<T>::has_trivial_ctor && traits<U>::has_trivial_ctor,
            has_trivial_dtor =
                    traits<T>::has_trivial_dtor && traits<U>::has_trivial_dtor,
            has_trivial_copy =
                    traits<T>::has_trivial_copy && traits<U>::has_trivial_copy,
            has_trivial_move =
                    traits<T>::has_trivial_move && traits<U>::has_trivial_move
        };
    };


#define DROID_TRIVIAL_CTOR_TRAIT( T ) \
    template<> struct trait_trivial_ctor< T > { enum { value = true }; };
#define DROID_TRIVIAL_DTOR_TRAIT( T ) \
    template<> struct trait_trivial_dtor< T > { enum { value = true }; };
#define DROID_TRIVIAL_COPY_TRAIT( T ) \
    template<> struct trait_trivial_copy< T > { enum { value = true }; };
#define DROID_TRIVIAL_MOVE_TRAIT( T ) \
    template<> struct trait_trivial_move< T > { enum { value = true }; };


#define DROID_BASIC_TYPES_TRAITS( T ) \
    DROID_TRIVIAL_CTOR_TRAIT( T )     \
    DROID_TRIVIAL_DTOR_TRAIT( T )     \
    DROID_TRIVIAL_COPY_TRAIT( T )     \
    DROID_TRIVIAL_MOVE_TRAIT( T )     \


    /**
     * basic types traits
     * */
    DROID_BASIC_TYPES_TRAITS( void )
    DROID_BASIC_TYPES_TRAITS( bool )
    DROID_BASIC_TYPES_TRAITS( char )
    DROID_BASIC_TYPES_TRAITS( unsigned char)
    DROID_BASIC_TYPES_TRAITS( short )
    DROID_BASIC_TYPES_TRAITS( unsigned short )
    DROID_BASIC_TYPES_TRAITS( int )
    DROID_BASIC_TYPES_TRAITS( unsigned int )
    DROID_BASIC_TYPES_TRAITS( long )
    DROID_BASIC_TYPES_TRAITS( unsigned long )
    DROID_BASIC_TYPES_TRAITS( unsigned long long )
    DROID_BASIC_TYPES_TRAITS( float )
    DROID_BASIC_TYPES_TRAITS( double )



    template <typename TYPE> inline
    void construct_type(TYPE* p, size_t n) {
        if (!traits<TYPE>::has_trivial_ctor) {
            while (n > 0) {
                n--;
                new(p++) TYPE;
            }
        }
    }

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

    template <typename TYPE> inline
    void splat_type(TYPE* where, const TYPE* what, size_t n) {
        if (!traits<TYPE>::has_trivial_copy) {
            while (n > 0) {
                n--;
                new(where) TYPE(*what);
                where++;
            }
        } else {
            while (n > 0) {
                n--;
                *where++ = *what;
            }
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

    template<typename TYPE> inline
    int strictly_order_type(const TYPE& lhs, const TYPE& rhs) {
        return (lhs < rhs) ? 1 : 0;
    }

    template<typename TYPE> inline
    int compare_type(const TYPE& lhs, const TYPE& rhs) {
        return
            strictly_order_type(rhs, lhs) - strictly_order_type(lhs, rhs);
    }


    // -------------------------------------------------------------------
    template<typename KEY, typename VALUE>
    struct key_value_pair_t {
        typedef KEY      key_t;
        typedef VALUE    value_t;

        KEY key;
        VALUE value;

        key_value_pair_t() {}
        explicit key_value_pair_t(const KEY& k) : key(k) {}
        inline bool operator < (const key_value_pair_t& o) const {
            return strictly_order_type(key, o.key);
        }
    };

    template<typename K, typename V>
    struct trait_trivial_ctor<key_value_pair_t<K, V>> {
        enum { value = aggregate_traits<K, V>::has_trivial_ctor };
    };
    template<typename K, typename V>
    struct trait_trivial_dtor<key_value_pair_t<K, V>> {
        enum { value = aggregate_traits<K, V>::has_trivial_dtor };
    };
    template<typename K, typename V>
    struct trait_trivial_copy<key_value_pair_t<K, V>> {
        enum { value = aggregate_traits<K, V>::has_trivial_copy };
    };
    template<typename K, typename V>
    struct trait_trivial_move<key_value_pair_t<K, V>> {
        enum { value = aggregate_traits<K, V>::has_trivial_move };
    };

} // namespace droid

#endif // DROID_TYPE_HELPERS_H