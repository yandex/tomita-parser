#pragma once

#include "deque.h"
#include "vector.h"
#include "utility.h"

#include <util/str_stl.h>

#include <stlport/queue>

template <class T, class S = ydeque<T> >
class yqueue: public NStl::queue<T, S> {
    typedef NStl::queue<T, S> TBase;
public:
    inline yqueue() {
    }

    explicit yqueue(const S& ss)
        : TBase(ss)
    {
    }

    inline void swap(yqueue& q) throw () {
        this->c.swap(q.c);
    }

    inline void clear() {
        this->c.clear();
    }

    inline S& Container() {
        return this->c;
    }

    inline const S& Container() const {
        return this->c;
    }
};

template <class T, class S = yvector<T>, class C = TLess<T> >
class ypriority_queue: public NStl::priority_queue<T, S, C> {
public:
    typedef NStl::priority_queue<T, S, C> TBase;

    inline ypriority_queue() {
    }

    explicit ypriority_queue(const C& x)
        : TBase(x)
    {
    }

    inline ypriority_queue(const C& x, const S& s)
        : TBase(x, s)
    {
    }

    template <class I>
    inline ypriority_queue(I f, I l)
        : TBase(f, l)
    {
    }

    template <class I>
    inline ypriority_queue(I f, I l, const C& x)
        : TBase(f, l, x)
    {
    }

    template <class I>
    inline ypriority_queue(I f, I l, const C& x, const S& s)
        : TBase(f, l, x, s)
    {
    }

    inline void clear() {
        this->c.clear();
    }

    inline void swap(ypriority_queue& pq) {
        this->c.swap(pq.c);
        DoSwap(this->comp, pq.comp);
    }

    inline S& Container() {
        return this->c;
    }

    inline const S& Container() const {
        return this->c;
    }
};
