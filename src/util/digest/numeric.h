#pragma once

#include <util/system/defaults.h>
#include <util/generic/typelist.h>

/*
 * from http://www.cris.com/~Ttwang/tech/inthash.htm
 */

static inline ui8 IntHashImpl(ui8 key8) throw () {
    size_t key = key8;

    key += ~(key << 15);
    key ^= (key >> 10);
    key += (key << 3);
    key ^= (key >> 6);
    key += ~(key << 11);
    key ^= (key >> 16);

    return static_cast<ui8>(key);
}

static inline ui16 IntHashImpl(ui16 key16) throw () {
    size_t key = key16;

    key += ~(key << 15);
    key ^= (key >> 10);
    key += (key << 3);
    key ^= (key >> 6);
    key += ~(key << 11);
    key ^= (key >> 16);

    return static_cast<ui16>(key);
}

static inline ui32 IntHashImpl(ui32 key) throw () {
    key += ~(key << 15);
    key ^= (key >> 10);
    key += (key << 3);
    key ^= (key >> 6);
    key += ~(key << 11);
    key ^= (key >> 16);

    return key;
}

static inline ui64 IntHashImpl(ui64 key) throw () {
    key += ~(key << 32);
    key ^= (key >> 22);
    key += ~(key << 13);
    key ^= (key >> 8);
    key += (key << 3);
    key ^= (key >> 15);
    key += ~(key << 27);
    key ^= (key >> 31);

    return key;
}

#ifdef _darwin_
static inline unsigned long IntHashImpl(unsigned long key) throw() {
    return IntHashImpl((ui32)key);
}
#endif

template <class T>
static inline T IntHash(T t) throw () {
    typedef typename TCommonUnsignedInts::template TSelectBy<TSizeOfPredicate<sizeof(T)>::template TResult>::TResult TCvt;

    return IntHashImpl((TCvt)(t));
}

/*
 * can handle floats && pointers
 */
template <class T>
static inline size_t NumericHash(T t) throw () {
    typedef typename TCommonUnsignedInts::template TSelectBy<TSizeOfPredicate<sizeof(T)>::template TResult>::TResult TCvt;

    union HIDDEN_VISIBILITY {
        TCvt cvt;
        T t;
    } u;

    u.t = t;

    return (size_t)IntHash(u.cvt);
}

template <class T>
static inline T CombineHashes(T l, T r) throw () {
    return IntHash(l) ^ r;
}
