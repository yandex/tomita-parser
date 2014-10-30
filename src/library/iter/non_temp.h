#pragma once

#include <util/generic/typetraits.h>

namespace NIter {

template <typename T, bool IsRef>
class TTempValueHolderBase {
public:
    void Reset(T value) {
        TmpValue = value;
    }

    T& GetRef() {
        return TmpValue;
    }
private:
    T TmpValue;
};

template <typename TRef>
class TTempValueHolderBase<TRef, true> {
public:
    TTempValueHolderBase()
        : TmpValue(NULL)
    {
    }

    void Reset(TRef value) {
        TmpValue = &value;
    }

    TRef GetRef() {
        return *TmpValue;
    }
private:
    typedef typename TTypeTraits<TRef>::TReferenceTo T;
    T* TmpValue;
};

template <typename T>
class TTempValueHolder: public TTempValueHolderBase<T, TTypeTraits<T>::IsReference> {
};

// An iterator always returning a reference to value, not a temporary copy
template <typename TBaseIt>
class TNonTempIterator: public TBaseIt {
    typedef typename TBaseIt::TValueRef TBaseValueRef;
    typedef typename TTypeTraits<TBaseValueRef>::TReferenceTo TValue;
public:
    TNonTempIterator() {
    }

    TNonTempIterator(TBaseIt it):
        TBaseIt(it)
    {
    }

    typedef TValue& TValueRef;

    TValue& operator*() {
        TmpValue.Reset(TBaseIt::operator*());
        return TmpValue.GetRef();
    }

    TValue* operator->() {
        return &operator*();
    }

private:
    using TBaseIt::operator*;       // hide

    TTempValueHolder<TBaseValueRef> TmpValue;
};

} // namespace NIter
