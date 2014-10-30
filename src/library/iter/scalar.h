#pragma once

#include "base.h"

namespace NIter {

template <class T>
class TScalarIterator: public TBaseIterator<T> {
public:
    // @value must exist during whole TScalarIterator's life-time
    inline explicit TScalarIterator(const T* value)
        : Value(value)
    {
    }

    virtual bool Ok() const override {
        return Value != NULL;
    }

    virtual void operator++() override {
        Value = NULL;
    }

    virtual const T* operator->() const override {
        return Value;
    }

private:
    const T* Value;
};

} // namespace NIter
