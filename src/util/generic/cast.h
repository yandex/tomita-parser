#pragma once

#include "typetraits.h"
#include "yexception.h"
#include "reinterpretcast.h"

#include <util/system/compat.h>
#include <util/system/yassert.h>

#include <cstdlib>
#include <typeinfo>

template <class T, class F>
static inline T VerifyDynamicCast(F f) {
    if (!f) {
        return 0;
    }

    T ret = dynamic_cast<T>(f);

    VERIFY(ret, "verify cast failed");

    return ret;
}

#if !defined(NDEBUG)
    #define USE_DEBUG_CHECKED_CAST
#endif

namespace NPrivate {
    template <typename T, typename F>
    static T DynamicCast(F f) {
        return dynamic_cast<T>(f);
    }
}

#define UTIL_PRIVATE_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

/*
 * replacement for dynamic_cast(dynamic_cast in debug mode, else static_cast)
 */
template <class T, class F>
static inline T CheckedCast(F f) {
#if defined(USE_DEBUG_CHECKED_CAST)
    return VerifyDynamicCast<T>(f);
#else
    // Make sure F is polymorphic.
    // Without this cast, CheckedCast with non-polymorphic F
    // incorrectly compiled without error in release mode.
#if !defined(__GNUC__) || UTIL_PRIVATE_GCC_VERSION >= 40700
    (void) ::NPrivate::DynamicCast<T, F>;
#endif

    return static_cast<T>(f);
#endif
}

/*
 * be polite
 */
#undef USE_DEBUG_CHECKED_CAST

template <bool isUnsigned>
class TInteger;

template <>
class TInteger<true> {
public:
    template <class TUnsigned>
    static bool IsNegative(TUnsigned) {
        return false;
    }
};

template <>
class TInteger<false> {
public:
    template <class TSigned>
    static bool IsNegative(TSigned value) {
        return value < 0;
    }
};

template <class TType>
inline bool IsNegative(TType value) {
    return TInteger<TTypeTraits<TType>::IsUnsignedInt>::IsNegative(value);
}

namespace NPrivate {
    template <class TSmallInt, class TLargeInt>
    struct TSafelyConvertible {
        enum {
            Result = (((TTypeTraits<TSmallInt>::IsSignedInt && TTypeTraits<TLargeInt>::IsSignedInt)
                    || (TTypeTraits<TSmallInt>::IsUnsignedInt && TTypeTraits<TLargeInt>::IsUnsignedInt))
                    && sizeof(TSmallInt) >= sizeof(TLargeInt))
                || (TTypeTraits<TSmallInt>::IsSignedInt && sizeof(TSmallInt) > sizeof(TLargeInt))
        };
    };
}

template <class TSmallInt, class TLargeInt>
inline typename TEnableIf< ::NPrivate::TSafelyConvertible<TSmallInt, TLargeInt>::Result, TSmallInt>::TResult SafeIntegerCast(TLargeInt largeInt) {
    return largeInt;
}

template <class TSmallInt, class TLargeInt>
inline typename TEnableIf< !::NPrivate::TSafelyConvertible<TSmallInt, TLargeInt>::Result, TSmallInt>::TResult SafeIntegerCast(TLargeInt largeInt) {
    if (TTypeTraits<TSmallInt>::IsUnsignedInt && TTypeTraits<TLargeInt>::IsSignedInt) {
        if (IsNegative(largeInt)) {
            throw TBadCastException() << "Conversion '"<< largeInt << "' to '"
                << typeid(TSmallInt).name()
                << "', negative value converted to unsigned";
        }
    }

    TSmallInt smallInt = TSmallInt(largeInt);

    if (TTypeTraits<TSmallInt>::IsSignedInt && TTypeTraits<TLargeInt>::IsUnsignedInt) {
        if (IsNegative(smallInt)) {
            throw TBadCastException() << "Conversion '"<< largeInt << "' to '"
                << typeid(TSmallInt).name()
                << "', positive value converted to negative";
        }
    }

    if (TLargeInt(smallInt) != largeInt) {
        throw TBadCastException() << "Conversion '" << largeInt << "' to '"
            << typeid(TSmallInt).name() << "', loss of data";
    }

    return smallInt;
}

template <class TSmallInt, class TLargeInt>
inline TSmallInt IntegerCast(TLargeInt largeInt) throw () {
    try {
        return SafeIntegerCast<TSmallInt>(largeInt);
    } catch (const yexception& exc) {
        FAIL("IntegerCast: %s", exc.what());
    }
}
