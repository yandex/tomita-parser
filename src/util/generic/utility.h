#pragma once

#include "typetraits.h"
#include "move.h"

#include <cstring>

template <class T>
static inline const T& Min(const T& l, const T& r) {
    if (l < r)
        return l;
    return r;
}

template <class T>
static inline const T& Max(const T& l, const T& r) {
    if (l > r)
        return l;
    return r;
}

template <class T>
inline T ClampVal(const T& val, const T& min, const T& max) {
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

template <class T>
static inline void Zero(T& t) throw () {
    memset(&t, 0, sizeof(t));
}

namespace NSwapCheck {
    HAS_MEMBER(swap)
    HAS_MEMBER(Swap)

    template <class T>
    static inline void DoSwapImpl(T& l, T& r) {
        T tmp(MoveArg(l));
        l = MoveArg(r);
        r = MoveArg(tmp);
    }

    template <bool val>
    struct TSmallSwapSelector {
        template <class T>
        static inline void Swap(T& l, T& r) {
            DoSwapImpl(l, r);
        }
    };

    template <>
    struct TSmallSwapSelector<true> {
        template <class T>
        static inline void Swap(T& l, T& r) {
            l.swap(r);
        }
    };

    template <bool val>
    struct TBigSwapSelector {
        template <class T>
        static inline void Swap(T& l, T& r) {
            TSmallSwapSelector<THasswap<T>::Result>::Swap(l, r);
        }
    };

    template <>
    struct TBigSwapSelector<true> {
        template <class T>
        static inline void Swap(T& l, T& r) {
            l.Swap(r);
        }
    };

    template <class T>
    class TSwapSelector: public TBigSwapSelector<THasSwap<T>::Result> {
    };
}

/*
 * DoSwap better than ::Swap in member functions...
 */
template <class T>
static inline void DoSwap(T& l, T& r) {
    NSwapCheck::TSwapSelector<T>::Swap(l, r);
}

template <bool b>
struct TNullTmpl {
    template<class T>
    operator T() const {
        return (T)0;
    }
};
typedef TNullTmpl<0> TNull;
