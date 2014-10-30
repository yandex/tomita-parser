#pragma once

#include "stl_container.h"

#include <util/generic/typetraits.h>

namespace NIter {

template <class T, class TSourceIter, class TMapper>
class TMappedIterator {
public:
    typedef T TValue;

    TMappedIterator()
        : Source()
        , Mapper(NULL)
    {
    }

    TMappedIterator(TSourceIter iter, const TMapper* mapper)
        : Source(iter)
        , Mapper(mapper)
    {
    }

    inline bool Ok() const {
        return Source.Ok();
    }

    inline void operator++() {
        ++Source;
    }

    inline const T* operator->() const {
        return &((*Mapper)(*Source));     // Mapper() should return (const TValue&)
    }

    inline const T& operator*() const {
        return (*Mapper)(*Source);
    }
private:
    TSourceIter Source;
    const TMapper* Mapper;
};


// Allows using TMappedIterator with any object having operator[] as mapper.
template <class TSource, class TKey, class TValue>
class TIndexerToMapper {
public:
    inline TIndexerToMapper(TSource& source)
        : Source(&source)
    {
    }

    inline TValue& operator()(const TKey& key) const {
        return (*Source)[key];
    }
private:
    TSource* Source;
};

// Allows using TMappedIterator with yvector as mapper.
template <class T>
class TVectorToMapper: public TIndexerToMapper<typename TStlContainerSelector<T>::TVector, size_t, T> {
public:
    typedef typename TStlContainerSelector<T>::TVector TVector;
    typedef TIndexerToMapper<TVector, size_t, T> TBase;

    TVectorToMapper(TVector& vector)
        : TBase(vector)
    {
    }
};


// better version of TMappedIterator
// TMapper should define TResult by default
template <class TSourceIter, class TMapper, class TMapResult = typename TMapper::TResult>
class TMappedIterator2 {
public:
    // TMapper should define TResult
    typedef TMapResult TValueRef;
    typedef typename TTypeTraits<TValueRef>::TReferenceTo TValue;
    typedef TValue* TValuePtr;

    TMappedIterator2()
        : Source()
        , Mapper(NULL)
    {
    }

    // TMapper should be a light object, easyly copyable
    TMappedIterator2(TSourceIter iter, const TMapper& mapper = TMapper())
        : Source(iter)
        , Mapper(mapper)
    {
    }

    bool Ok() const {
        return Source.Ok();
    }

    void operator++() {
        ++Source;
    }

    TValueRef operator*() const {
        return Mapper(*Source);
    }

private:
    TSourceIter Source;
    TMapper Mapper;
};

} // namespace NIter
