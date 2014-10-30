#pragma once

#include <util/memory/alloc.h>

#include <stlport/deque>

template <class T, class A = DEFAULT_ALLOCATOR(T)>
class ydeque: public NStl::deque<T, REBIND_ALLOCATOR(A, T)> {
public:
    typedef NStl::deque<T, REBIND_ALLOCATOR(A, T)> TBase;
    typedef typename TBase::size_type size_type;

    inline ydeque()
        : TBase()
    {
    }

    inline ydeque(const typename TBase::allocator_type& a)
        : TBase(a)
    {
    }

    explicit inline ydeque(size_type count)
        : TBase(count)
    {
    }

    inline ydeque(size_type count, const T& val)
        : TBase(count, val)
    {
    }

    template <class TIter>
    inline ydeque(TIter first, TIter last)
        : TBase(first, last)
    {
    }

    inline size_type operator+ () const throw () {
        return this->size();
    }
};
