#pragma once

#include "stl_container.h"

namespace NIter {

template <class T>
class TSetIterator: public TStlContainerIterator<typename TStlContainerSelector<T>::TSet> {
    typedef typename TStlContainerSelector<T>::TSet TSet;
    typedef TStlContainerIterator<TSet> TBase;
public:

    inline TSetIterator()
        : TBase()
    {
    }

    inline explicit TSetIterator(TSet& set)
        : TBase(set)
    {
    }

    inline TSetIterator(typename TBase::TStlIter it_begin, typename TBase::TStlIter it_end)
        : TBase(it_begin, it_end)
    {
    }
};

} // namespace NIter
