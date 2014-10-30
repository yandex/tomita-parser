#pragma once

#include "yassert.h"
#include "defaults.h"

template <class T>
static inline T AlignDown(T len, T align) throw () {
    YASSERT((align & (align - 1)) == 0);  // align should be power of 2
    return len & ~(align - 1);
}

template <class T>
static inline T AlignUp(T len, T align) throw () {
    return AlignDown(len + (align - 1), align);
}

template <class T>
static inline T AlignUpSpace(T len, T align) throw () {
    YASSERT((align & (align - 1)) == 0);  // align should be power of 2
    return ((T)0 - len) & (align - 1);  // AlignUp(len, align) - len;
}

template <class T>
static inline T* AlignUp(T* ptr, size_t align) throw () {
    return (T*)AlignUp((size_t)ptr, align);
}

template <class T>
static inline T* AlignDown(T* ptr, size_t align) throw () {
    return (T*)AlignDown((size_t)ptr, align);
}

template <class T>
static inline T AlignUp(T t) throw () {
    return AlignUp(t, (size_t)PLATFORM_DATA_ALIGN);
}

template <class T>
static inline T AlignDown(T t) throw () {
    return AlignDown(t, (size_t)PLATFORM_DATA_ALIGN);
}

template <class T>
static inline T Align(T t) throw () {
    return AlignUp(t);
}

union TAligner {
    void*                  Data1;
    long double            Data2;
    unsigned long long int Data3;
};
