#pragma once

#include "vector.h"

#include <util/generic/typetraits.h>

namespace NIter {

// same as TVectorIterator, but can also be initialized on non-ptr single value
// (it makes a copy of this value and refers to it with IterCur.
// TODO: think of more appropriate class name!
template <class T>
class TValueIterator: public TVectorIterator<T>
{
    typedef TVectorIterator<T> TBase;
public:

    inline TValueIterator()
        : TBase()
    {
    }

    inline TValueIterator(typename TBase::TStlIter it_begin, typename TBase::TStlIter it_end)
        : TBase(it_begin, it_end)
    {
    }

    inline explicit TValueIterator(const T& value)
        : Value(value)
    {
        Reset(&Value, &Value + 1);
    }

    inline TValueIterator(const TValueIterator& src)
        : TBase(src), Value(src.Value)
    {
        if (src.Cur == &src.Value)
            Reset(&Value, &Value + 1);
    }

    inline TValueIterator& operator=(const TValueIterator& src) {
        *(TBase*)this = src;
        Value = src.Value;
        if (src.Cur == &src.Value)
            Reset(&Value, &Value + 1);
        return *this;
    }

    using TBase::Reset;

    inline void Reset(const T& value) {
        Value = value;
        Reset(&Value, &Value + 1);
    }

private:
    typename TTypeTraits<T>::TNonConst Value;
};

} // namespace NGzt
