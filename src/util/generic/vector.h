#pragma once

#include <util/memory/alloc.h>

#include <stlport/vector>

template <class T, class A = DEFAULT_ALLOCATOR(T)>
class yvector: public NStl::vector<T, REBIND_ALLOCATOR(A, T)> {
public:
    typedef NStl::vector<T, REBIND_ALLOCATOR(A, T)> TBase;
    typedef typename TBase::size_type size_type;

    inline yvector()
        : TBase()
    {
    }

    inline yvector(const typename TBase::allocator_type& a)
        : TBase(a)
    {
    }

    inline explicit yvector(size_type count)
        : TBase(count)
    {
    }

    inline yvector(size_type count, const T& val)
        : TBase(count, val)
    {
    }

    inline yvector(size_type count, const T& val,
                   const typename TBase::allocator_type& a)
        : TBase(count, val, a)
    {
    }

    template <class TIter>
    inline yvector(TIter first, TIter last)
        : TBase(first, last)
    {
    }

    inline size_type operator+ () const throw () {
        return this->size();
    }

    inline T* operator~ () throw () {
        return this->data();
    }

    inline const T* operator~ () const throw () {
        return this->data();
    }

    inline int ysize() const throw () {
        return (int)TBase::size();
    }

    inline void shrink_to_fit() {
        yvector<T, A>(*this).swap(*this);
    }

    //compat methods
    inline void push_back_old(const T& t) {
        this->push_back(t);
    }
};
