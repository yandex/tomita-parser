#pragma once

#include "base.h"

#include <util/generic/yexception.h>

namespace NIter {

template <class T>
class TEmptyIterator: public TBaseIterator<T> {
public:
    // implements TBaseIterator
    virtual bool Ok() const override {
        return false;
    }

    virtual void operator++() override {
    }

    virtual const T* operator->() const override {
        ythrow yexception() << "Dereferencing empty iterator!";
    }
};

} // namespace NIter
