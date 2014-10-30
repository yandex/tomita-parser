#pragma once

#include "base.h"

namespace NIter {

template <class T>
class TRangeIterator: public TBaseIterator<T> {
public:
    inline TRangeIterator(const T* values, size_t size)
        : Cur(values)
        , End(values + size)
    {
    }

    inline TRangeIterator(const T* value_begin, const T* value_end)
        : Cur(value_begin)
        , End(value_end)
    {
    }

    virtual bool Ok() const override {
        return Cur != End;
    }

    virtual void operator++() override {
        ++Cur;
    }

    virtual const T* operator->() const override {
        return Cur;
    }

private:
    const T *Cur, *End;
};

} // namespace NIter
