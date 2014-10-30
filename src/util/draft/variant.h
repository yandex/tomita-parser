#pragma once

#include "align.h"

#include <util/stream/output.h>

#include <util/system/defaults.h>

#include <util/str_stl.h>

#include <string.h>

// TODO - move to common place
#ifndef MAX
#   define MAX(a,b) ((a)>(b)?(a):(b))
#endif

namespace NVariantDetail {

template<class T, class T1, class T2> class TGet;

}


// no strong (transactional) guarantee for assignment/copying
template<class T1, class T2> // TODO - extend to more types
class TVariant
{
private:
    bool Second;

    union
    {
        TAlignedMemoryFor<T1> Mem1;
        TAlignedMemoryFor<T2> Mem2;
    };

private:
    // w/o Clear
    void Assign(const TVariant& rhs)
    {
        Second = rhs.Second;
        if (Second) {
            new(Mem2.Mem) T2(*rhs.Get<T2>());
        } else {
            new(Mem1.Mem) T1(*rhs.Get<T1>());
        }
    }

    void Clear()
    {
        if (Second) {
            Mem2.Get()->~T2();
        } else {
            Mem1.Get()->~T1();
        }
    }

    template<class T, class T11, class T21>  friend class NVariantDetail::TGet;

    template<class T11, class T21>
    friend TOutputStream& operator<<(TOutputStream& o, const TVariant<T11,T21>& v);

public:
    TVariant(const T1& rhs)
      : Second(false)
    {
        new(Mem1.Mem) T1(rhs);
    }

    TVariant(const T2& rhs)
      : Second(true)
    {
        new(Mem2.Mem) T2(rhs);
    }

    TVariant(const TVariant& rhs)
    {
        Assign(rhs);
    }

    ~TVariant()
    {
        Clear();
    }

    TVariant& operator=(const T1& rhs)
    {
        if (&rhs != Mem1.Get()) {
            Clear();
            Second = false;
            new(Mem1.Mem) T1(rhs);
        }
        return *this;
    }

    TVariant& operator=(const T2& rhs)
    {
        if (&rhs != &Mem2.Get()) {
            Clear();
            Second = true;
            new(Mem2.Mem) T2(rhs);
        }
        return *this;
    }

    TVariant& operator=(const TVariant& rhs)
    {
        if (&rhs != this) {
            Clear();
            Assign(rhs);
        }
        return *this;
    }

    bool operator==(const TVariant& rhs) const
    {
        if (Second != rhs.Second)
            return false;

        return Second ? (*Mem2.Get() == *rhs.Mem2.Get())
                      : (*Mem1.Get() == *rhs.Mem1.Get());
    }

    bool operator==(const T1& rhs) const
    {
        if (!IsSecond()) {
            return *Mem1.Get() == rhs;
        }
        return false;
    }

    bool operator==(const T2& rhs) const
    {
        if (IsSecond()) {
            return *Mem2.Get() == rhs;
        }
        return false;
    }

    bool IsSecond() const
    {
        return Second;
    }

    template<class T>
    T* Get()
    {
        return NVariantDetail::TGet<T,T1,T2>::Get(*this);
    }

    template<class T>
    const T* Get() const
    {
        return NVariantDetail::TGet<T,T1,T2>::Get(*this);
    }
};

namespace NVariantDetail {

template<class T1, class T2> // TODO - extend to more types
class TGet<T1,T1,T2>
{
public:
    static T1* Get(TVariant<T1,T2>& v)
    {
        return !v.Second ? v.Mem1.Get() : NULL;
    }
    static const T1* Get(const TVariant<T1,T2>& v)
    {
        return !v.Second ? v.Mem1.Get() : NULL;
    }
};

template<class T1, class T2> // TODO - extend to more types
class TGet<T2,T1,T2>
{
public:
    static T2* Get(TVariant<T1,T2>& v)
    {
        return v.Second ? v.Mem2.Get() : NULL;
    }
    static const T2* Get(const TVariant<T1,T2>& v)
    {
        return v.Second ? v.Mem2.Get() : NULL;
    }
};

}

template<class T1, class T2>
TOutputStream& operator<<(TOutputStream& o, const TVariant<T1,T2>& v)
{
    if (v.IsSecond()) {
        o << *v.Mem2.Get();
    } else {
        o << *v.Mem1.Get();
    }
    return o;
}


template<class T1, class T2>
struct THash< TVariant<T1, T2> >
{
private:
    THash<T1> Hash1;
    THash<T2> Hash2;

public:
    inline size_t operator()(const TVariant<T1, T2>& v) const {
        if (v.IsSecond()) {
            return ~Hash2(*v.template Get<T2>());
        } else {
            return Hash1(*v.template Get<T1>());
        }
    }
};


