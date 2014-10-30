#pragma once

namespace NIter {

// If T is const then iterator would return only const reference to iterated items
template <class TIter, class T>
class TBeginEndIterator {
public:
    typedef T TValue;
    typedef T& TValueRef;
    typedef T* TValuePtr;

    inline TBeginEndIterator()
        : Cur()
        , End()
    {
    }

    inline TBeginEndIterator(TIter it_begin, TIter it_end)
        : Cur(it_begin)
        , End(it_end)
    {
    }

    inline void Reset(TIter it_begin, TIter it_end) {
        Cur = it_begin;
        End = it_end;
    }

    inline TIter begin() const {
        return Cur;
    }

    inline TIter end() const {
        return End;
    }

    inline bool Ok() const {
        return Cur != End;
    }

    inline void operator++() {
        ++Cur;
    }

    inline TValuePtr operator->() const {
        return &*Cur;
    }

    inline TValueRef operator*() const {
        return *Cur;
    }
protected:
    TIter Cur, End;
};

} // namespace NIter
