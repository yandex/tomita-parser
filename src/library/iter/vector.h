#pragma once

#include "stl_container.h"

namespace NIter {

template <class T>
class TVectorIterator: public TStlContainerIterator<typename TStlContainerSelector<T>::TVector> {
    typedef typename TStlContainerSelector<T>::TVector TVector;
    typedef TStlContainerIterator<TVector> TBase;

public:
    inline TVectorIterator()
        : TBase()
    {
    }

    inline explicit TVectorIterator(TVector& vector)
        : TBase(vector)
    {
    }

    inline TVectorIterator(typename TBase::TStlIter it_begin, typename TBase::TStlIter it_end)
        : TBase(it_begin, it_end)
    {
    }

    inline explicit TVectorIterator(T* value, size_t count)
        : TBase(value, value + count)
    {
    }

    inline T* operator->() const {
        return TBase::Cur;    // should be more efficient than &*IterCur (?)
    }

    inline size_t Size() const {
        return TBase::End - TBase::Cur;
    }
};

} // namespace NIter
