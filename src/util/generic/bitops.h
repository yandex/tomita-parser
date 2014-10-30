#pragma once

#include "ylimits.h"
#include "typetraits.h"

#include <util/system/yassert.h>

namespace NBitOps {
    namespace NPrivate {
        template <unsigned N, class T>
        struct TClp2Helper {
            static inline T Calc(T t) throw () {
                const T prev = TClp2Helper<N / 2, T>::Calc(t);

                return prev | (prev >> N);
            }
        };

        template <class T>
        struct TClp2Helper<0u, T> {
            static inline T Calc(T t) throw () {
                return t - 1;
            }
        };
    }
}

/**
 * Compute the next highest power of 2 of integer paramter `t`.
 * Result is undefined for `t == 0`.
 */
template <class T>
static inline T FastClp2(T t) throw () {
    YASSERT(t > 0);

    return 1 + NBitOps::NPrivate::TClp2Helper<sizeof(T) * 4, T>::Calc(t);
}

template <class T>
static inline T FastClp22(T t) throw () {
    if (EXPECT_TRUE(t & (t - 1)))
        return FastClp2(t);

    return t;
}

#if defined(__GNUC__) && __GNUC__ > 2

inline unsigned GetValueBitCountImpl(unsigned int value) {
    YASSERT(value); // because __builtin_clz* have undefined result for zero.
    return TNumericLimits<unsigned int>::digits - __builtin_clz(value);
}

inline unsigned GetValueBitCountImpl(unsigned long value) {
    YASSERT(value); // because __builtin_clz* have undefined result for zero.
    return TNumericLimits<unsigned long>::digits - __builtin_clzl(value);
}

inline unsigned GetValueBitCountImpl(unsigned long long value) {
    YASSERT(value); // because __builtin_clz* have undefined result for zero.
    return TNumericLimits<unsigned long long>::digits - __builtin_clzll(value);
}

inline unsigned CountTrailingZeroBits(unsigned int value) {
    YASSERT(value); // because __builtin_ctz* have undefined result for zero.
    return __builtin_ctz(value);
}

inline unsigned CountTrailingZeroBits(unsigned long value) {
    YASSERT(value); // because __builtin_ctz* have undefined result for zero.
    return __builtin_ctzl(value);
}

inline unsigned CountTrailingZeroBits(unsigned long long value) {
    YASSERT(value); // because __builtin_ctz* have undefined result for zero.
    return __builtin_ctzll(value);
}

#else

/// Stupid realization for non-GCC. Can use BSR from x86 instructions set.
template <typename T>
inline unsigned GetValueBitCountImpl(T value) {
    YASSERT(value); // because __builtin_clz* have undefined result for zero.
    unsigned result = 1; // result == 0 - impossible value, see YASSERT().
    value >>= 1;
    while (value) {
        value >>= 1;
        result++;
    }
    return result;
}

/// Stupid realization for non-GCC. Can use BSF from x86 instructions set.
template <typename T>
inline unsigned CountTrailingZeroBits(T value) {
    YASSERT(value); // because __builtin_ctz* have undefined result for zero.
    unsigned result = 0;
    while (!(value & 1)) {
        value >>= 1;
        ++result;
    }
    return result;
}

#endif

template <typename T>
inline unsigned GetValueBitCount(T value) {
    typedef typename TUnsignedInts::template TSelectBy<TSizeOfPredicate<sizeof(T)>::template TResult>::TResult TCvt;

    return GetValueBitCountImpl((TCvt)value);
}

#if defined(__GNUC__) && __GNUC__ > 2
inline unsigned CountBitsImpl(unsigned v) {
    return __builtin_popcount(v);
}

inline unsigned CountBitsImpl(unsigned long v) {
    return __builtin_popcountl(v);
}

inline unsigned CountBitsImpl(unsigned long long v) {
    return __builtin_popcountll(v);
}
#else
template <class T>
static unsigned CountBitsImpl(T v) {
    unsigned res = 0;

    for (; v; v >>= 1) {
        if (v & 1) {
            ++res;
        }
    }

    return res;
}
#endif

template <class T>
inline size_t CountBits(T v) {
    typedef typename TUnsignedInts::template TSelectBy<TSizeOfPredicate<sizeof(T)>::template TResult>::TResult TCvt;

    return CountBitsImpl((TCvt)v);
}
