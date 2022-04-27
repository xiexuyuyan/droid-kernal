#ifndef DROID_KEYED_VECTOR_H
#define DROID_KEYED_VECTOR_H

#include "utils/SortedVector.h"

namespace droid {
    template<typename KEY, typename VALUE>
    class KeyedVector {
    public:
        typedef KEY   key_type;
        typedef VALUE value_type;

        inline        KeyedVector();

        inline  size_t          size() const { return mVector.size(); }
        inline  bool            isEmpty() const { return mVector.isEmpty(); };
        const   VALUE&          valueAt(size_t index) const;
                ssize_t         indexOfKey(const KEY& key) const;

    private:
        SortedVector<key_value_pair_t<KEY, VALUE>> mVector;
    };


    template<typename KEY, typename VALUE> inline
    KeyedVector<KEY, VALUE>::KeyedVector() {
        // empty
    }

    template<typename KEY, typename VALUE> inline
    const VALUE& KeyedVector<KEY, VALUE>::valueAt(size_t index) const {
        return mVector.itemAt(index).value;
    }


    template<typename KEY, typename VALUE> inline
    ssize_t KeyedVector<KEY, VALUE>::indexOfKey(const KEY& key) const {
        return mVector.indexOf(key_value_pair_t<KEY, VALUE>(key));
    }


} // namespace droid



#endif // DROID_KEYED_VECTOR_H