#pragma once

#include "deque.h"

#include <stlport/stack>

template <class T, class S = ydeque<T> >
class ystack: public NStl::stack<T, S> {
    typedef NStl::stack<T, S> TBase;
public:
    inline ystack()
        : TBase()
    {
    }

    explicit ystack(const S& s)
        : TBase(s)
    {
    }
};
