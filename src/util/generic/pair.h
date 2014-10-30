#pragma once

#include <util/system/defaults.h>

#include <stlport/utility>

template <class T1, class T2 = T1>
struct TPair : NStl::pair<T1, T2> {
    typedef T1 TFirst;
    typedef T2 TSecond;

    TPair() {
    }

    TPair(const T1& x, const T2& y)
        : NStl::pair<T1, T2> (x, y)
    {
    }

    template <class TU, class TV>
    TPair(const NStl::pair<TU, TV>& p)
        : NStl::pair<T1, T2>(p)
    {
    }

    template <class TU, class TV>
    TPair(const TPair<TU, TV>& p)
        : NStl::pair<T1, T2>(p)
    {
    }

    template <class TU, class TV>
    TPair& operator=(const TPair<TU, TV>& p) {
        this->first = p.first;
        this->second = p.second;
        return *this;
    }

    TFirst& First() {
        return this->first;
    }

    TSecond& Second() {
        return this->second;
    }

    const TFirst& First() const {
        return this->first;
    }

    const TSecond& Second() const {
        return this->second;
    }
};

template<class T1, class T2>
inline TPair<T1, T2> MakePair(T1 v1, T2 v2) {// return pair composed from arguments
    return (TPair<T1, T2>(v1, v2));
}

template<class T>
inline TPair<T, T> MakePair(T v) {// return pair composed from argument
    return (TPair<T, T>(v, v));
}

template <class T>
class TTypeTraits;

template <class T>
struct TPodTraits;

template <class T1, class T2>
struct TPodTraits<TPair<T1, T2> > {
    enum {
        IsPod = TTypeTraits<T1>::IsPod && TTypeTraits<T2>::IsPod
    };
};

struct TSwallowAssign {
    template <class T>
    const TSwallowAssign& operator=(const T&) const {
        return *this;
    }
};

extern TSwallowAssign TieIgnore;

template <class T1, class T2>
inline TPair<T1&, T2&> Tie(T1&& t1, T2&& t2) {
    return TPair<T1&, T2&>(t1, t2);
}
