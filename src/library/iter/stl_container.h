#pragma once

#include "begin_end.h"

#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/generic/typehelpers.h>
#include <util/generic/typetraits.h>

namespace NIter {

template <class TContainer>
class TStlContainerTypeTraits {
    typedef typename TContainer::value_type TValueType;
    typedef typename TTypeTraits<TContainer>::TNonConst TNonConstContainer;
    //typedef typename TNonConstContainer<TNonConstValueType> TBaseContainer;

public:
    typedef typename TTypeTraits<TValueType>::TNonConst TNonConstValueType;
    typedef const TNonConstValueType TConstValueType;

    typedef typename TNonConstContainer::const_iterator TConstIter;
    typedef typename TNonConstContainer::iterator TNonConstIter;

    enum { IsConst = TTypeTraits<TContainer>::IsConstant };

public:
    typedef typename TSelectType<IsConst, TConstValueType, TNonConstValueType >::TResult TValue;
    typedef typename TSelectType<IsConst, TConstIter, TNonConstIter >::TResult TIterator;

    //typedef typename TSelectType<IsConst, const TBaseContainer, TBaseContainer>::TResult TDefaultContainer;
};

template <class T>
class TStlContainerSelector {
    enum { IsConst = TTypeTraits<T>::IsConstant };
    typedef typename TTypeTraits<T>::TNonConst TNonConst;
public:
    typedef typename TSelectType<IsConst, const yvector<TNonConst>, yvector<TNonConst> >::TResult TVector;
    typedef typename TSelectType<IsConst, const yset<TNonConst>, yset<TNonConst> >::TResult TSet;
};


// If TContainer is const then iterator would return only const reference to iterated items
template <class TContainer>
class TStlContainerIterator: public TBeginEndIterator<typename TStlContainerTypeTraits<TContainer>::TIterator,
                                                      typename TStlContainerTypeTraits<TContainer>::TValue> {
public:
    typedef typename TStlContainerTypeTraits<TContainer>::TValue TValue;
    typedef typename TStlContainerTypeTraits<TContainer>::TIterator TStlIter;
    typedef TBeginEndIterator<TStlIter, TValue> TBase;

    inline TStlContainerIterator()
        : TBase()
    {
    }

    inline explicit TStlContainerIterator(TContainer& container)
        : TBase(container.begin(), container.end())
    {
    }

    inline TStlContainerIterator(TStlIter it_begin, TStlIter it_end)
        : TBase(it_begin, it_end)
    {
    }

    using TBeginEndIterator<TStlIter, TValue>::Reset;

    inline void Reset(TContainer& container) {
        TBase::Reset(container.begin(), container.end());
    }
};

} // namespace NIter
