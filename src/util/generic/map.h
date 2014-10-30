#pragma once

#include "pair.h"
#include "mapfindptr.h"

#include <util/memory/alloc.h>
#include <util/str_stl.h>

#include <stlport/map>

template <class K, class V, class Less = TLess<K>, class A = DEFAULT_ALLOCATOR(K)>
class ymap: public NStl::map<K, V, Less, REBIND_ALLOCATOR(A, FIX2ARG(TPair<K, V>))>, public TMapOps<V, ymap<K, V, Less, A> > {
    typedef NStl::map<K, V, Less, REBIND_ALLOCATOR(A, FIX2ARG(TPair<K, V>))> TBase;
public:
    inline ymap() {
    }

    template <typename TAllocParam>
    inline explicit ymap(TAllocParam* allocator)
        : TBase(Less(), allocator)
    {
    }

    template <class It>
    inline ymap(It f, It l)
        : TBase(f, l)
    {
    }

    inline bool has(const K& key) const {
        return this->find(key) != this->end();
    }
};

template <class K, class V, class Less = TLess<K>, class A = DEFAULT_ALLOCATOR(K)>
class ymultimap: public NStl::multimap<K, V, Less, REBIND_ALLOCATOR(A, FIX2ARG(TPair<K, V>))> {
    typedef NStl::multimap<K, V, Less, REBIND_ALLOCATOR(A, FIX2ARG(TPair<K, V>))> TBase;
    typedef typename TBase::allocator_type TBaseAllocatorType;
public:
    inline ymultimap() {
    }

    template <typename TAllocParam>
    inline explicit ymultimap(TAllocParam* allocator)
        : TBase(Less(), allocator)
    {
    }

    inline explicit ymultimap(const Less& less, const TBaseAllocatorType& alloc = TBaseAllocatorType())
        : TBase(less, alloc)
    {
    }

    template <class It>
    inline ymultimap(It f, It l)
        : TBase(f, l)
    {
    }
};
