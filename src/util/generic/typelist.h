#pragma once

#include "typehelpers.h"
#include "static_assert.h"

/*
 * terminal type for typelists and typelist algorithms
 */
struct TNone {
    enum {
        Length = 0
    };

    template <class V>
    struct THave {
        enum {
            Result = false
        };
    };

    template <template <class> class P>
    struct TSelectBy {
        typedef TNone TResult;
    };
};

/*
 * type list, in all it's fame
 */
template <class H, class T>
struct TTypeList {
    typedef H THead;
    typedef T TTail;

    /*
     * length of type list
     */
    enum {
        Length = 1 + TTail::Length
    };

    template <class V>
    struct THave {
        enum {
            Result = TSameType<H, V>::Result || T::template THave<V>::Result
        };
    };

    template <template <class> class P>
    struct TSelectBy {
        typedef typename TSelectType<P<THead>::Result, THead, typename TTail::template TSelectBy<P>::TResult>::TResult TResult;
    };
};

template<class... R>
struct TTypeListBuilder;

template<>
struct TTypeListBuilder<> {
    typedef TNone Type;
};

template<class T, class... R>
struct TTypeListBuilder<T, R...>: TTypeList<T, typename TTypeListBuilder<R...>::Type> {
    typedef TTypeList<T, typename TTypeListBuilder<R...>::Type> Type;
};

/*
 * some very common typelists
 */
typedef TTypeListBuilder<signed char, signed short, signed int, signed long, signed long long> TCommonSignedInts;
typedef TTypeListBuilder<unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long, bool> TCommonUnsignedInts;
typedef TTypeListBuilder<float, double, long double> TFloats;

namespace NTL {
    template <bool isSigned, class T, class TS, class TU>
    struct TTypeSelectorBase {
        typedef TTypeList<T, TS> TSignedInts;
        typedef TU TUnsignedInts;
    };

    template <class T, class TS, class TU>
    struct TTypeSelectorBase<false, T, TS, TU> {
        typedef TS TSignedInts;
        typedef TTypeList<T, TU> TUnsignedInts;
    };

    template <class T, class TS, class TU>
    struct TTypeSelector: public TTypeSelectorBase<((T)(-1) < 0), T, TS, TU> {
    };

    typedef TTypeSelector<char, TCommonSignedInts, TCommonUnsignedInts> T1;
    typedef TTypeSelector<wchar_t, T1::TSignedInts, T1::TUnsignedInts> T2;
}

typedef NTL::T2::TSignedInts TSignedInts;
typedef NTL::T2::TUnsignedInts TUnsignedInts;

STATIC_ASSERT(TSignedInts::THave<char>::Result)

template <unsigned sizeOf>
struct TSizeOfPredicate {
    template <class T>
    struct TResult {
        enum {
            Result = (sizeof(T) == sizeOf)
        };
    };
};
