#pragma once

#include <util/generic/typetraits.h>
#include <util/generic/typehelpers.h>
#include <util/system/defaults.h>

namespace NIter {

template <class TIt1, class TIt2>
class TJoinIterator {
    // join sub-iterators should have TValueRef defined (which is a type returned by its operator*())
    typedef typename TIt1::TValueRef TValueRef1;
    typedef typename TIt2::TValueRef TValueRef2;
public:
    // use non-ref value if any
    typedef typename TSelectType<TTypeTraits<TValueRef1>::IsReference, TValueRef2, TValueRef1>::TResult TValueRef;
    typedef typename TTypeTraits<TValueRef>::TReferenceTo TValue;   // clean (non-ref) value type, could be equal to TValueRef
    typedef TValue* TValuePtr;

    inline TJoinIterator()
        : Active(0)
    {
    }

    inline TJoinIterator(TIt1 it1, TIt2 it2)
        : Active(1)
        , It1(it1)
        , It2(it2)
    {
        if (!It1.Ok())
            Active = It2.Ok() ? 2 : 0;
    }

    inline bool Ok() const {
        return Active != 0;
    }

    inline void operator++() {
        YASSERT(Ok());
        if (Active == 1) {
            ++It1;
            if (!It1.Ok())
                Active = 2;
        } else {
            ++It2;
        }
        if (!It2.Ok())
            Active = 0;
    }

    inline TValueRef operator*() const {
        if (Active == 1)
            return *It1;
        else
            return *It2;
    }

    inline TValuePtr operator->() const {
        if (Active == 1)
            return It1.operator->();
        else
            return It2.operator->();
    }

private:
    ui8 Active;
    TIt1 It1;
    TIt2 It2;
};


template <class TIt1, class TIt2, class TIt3>
class TJoinIterator3: public TJoinIterator<TIt1, TJoinIterator<TIt2, TIt3> >
{
    typedef TJoinIterator<TIt2, TIt3> TJoin;
    typedef TJoinIterator<TIt1, TJoin> TBase;
public:
    inline TJoinIterator3()
    {
    }

    inline TJoinIterator3(TIt1 it1, TIt2 it2, TIt3 it3)
        : TBase(it1, TJoin(it2, it3))
    {
    }
};

} // namespace NIter
