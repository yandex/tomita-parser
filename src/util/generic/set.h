#pragma once

#include <util/memory/alloc.h>
#include <util/str_stl.h>

#include <stlport/set>

template <class K, class L = TLess<K>, class A = DEFAULT_ALLOCATOR(K)>
class yset: public NStl::set<K, L, REBIND_ALLOCATOR(A, K)> {
    typedef NStl::set<K, L, REBIND_ALLOCATOR(A, K)> TBase;
public:
    inline yset() {
    }

    template <class It>
    inline yset(It f, It l)
        : TBase(f, l)
    {
    }

    template <class TheKey>
    inline bool has(const TheKey& key) const {
        return this->find(key) != this->end();
    }
};

template <class K, class L = TLess<K>, class A = DEFAULT_ALLOCATOR(K)>
class ymultiset: public NStl::multiset<K, L, REBIND_ALLOCATOR(A, K)> {
    typedef NStl::multiset<K, L, REBIND_ALLOCATOR(A, K)> TBase;
public:
    inline ymultiset() {
    }

    template <class It>
    inline ymultiset(It f, It l)
        : TBase(f, l)
    {
    }
};
