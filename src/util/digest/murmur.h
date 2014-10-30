#pragma once

#include <util/system/defaults.h>


/*
 * murmur2 from http://murmurhash.googlepages.com/
 *
 */
namespace NMurmurPrivate {
    ui32 MurmurHash32(const void* key, size_t len, ui32 seed) throw ();
    ui64 MurmurHash64(const void* key, size_t len, ui64 seed);

    template <unsigned N>
    struct TMurHelper;

#define DEF_MUR(t)\
    template <>\
    struct TMurHelper<t> {\
        static inline ui ## t MurmurHash(const void* buf, size_t len, ui ## t init) { \
            return MurmurHash ## t (buf, len, init);                        \
        }\
    };

    DEF_MUR(32)
    DEF_MUR(64)

#undef DEF_MUR
}

template <class T>
static inline T MurmurHash(const void* buf, size_t len, T init) {
    return (T)NMurmurPrivate::TMurHelper<8 * sizeof(T)>::MurmurHash(buf, len, init);
}

template <class T>
static inline T MurmurHash(const void* buf, size_t len) {
    return MurmurHash<T>(buf, len, (T)0);
}
