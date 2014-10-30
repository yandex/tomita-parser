#pragma once

#include <util/stream/output.h>
#include <util/datetime/base.h>

namespace NFormat {

template <typename T>
struct TSimpleCount {
    T Count;

    TSimpleCount(T count)
        : Count(count)
    {
    }
};

template <typename T>
inline TOutputStream& operator << (TOutputStream& o, const TSimpleCount<T>& f) {
    return o << f.Count;
}

TOutputStream& operator << (TOutputStream& o, const TSimpleCount<double>& f);



template <typename T>
struct TShortCount: public TSimpleCount<T> {
    T KBase;

    TShortCount(T count, T kbase)
        : TSimpleCount<T>(count)
        , KBase(kbase)
    {
    }
};

template <typename T>
TOutputStream& operator << (TOutputStream& o, const TShortCount<T>& f);

TOutputStream& operator << (TOutputStream& o, const TShortCount<TDuration>& f);



struct TPercents {
    double Value, Total;

    template <typename T>
    TPercents(T value, T total)
        : Value(value)
        , Total(total)
    {
    }

    TPercents(TDuration value, TDuration total)
        : Value(value.GetValue())
        , Total(total.GetValue())
    {
    }
};


TOutputStream& operator << (TOutputStream& o, const TPercents& f);



// Formatters

template <typename T>
inline TSimpleCount<T> Simple(T t) {
    return TSimpleCount<T>(t);
}

template <typename T>
inline TShortCount<T> Short(T count, T kbase = 1000) {
    return TShortCount<T>(count, kbase);
}

inline TShortCount<TDuration> Short(TDuration d) {
    return TShortCount<TDuration>(d, TDuration());
}

inline TShortCount<TDuration> Duration(TDuration d) {
    return Short(d);
}

inline TShortCount<size_t> Bytes(size_t bytes) {
    return Short<size_t>(bytes, 1024);
}

template <typename T>
inline TPercents Percents(T value, T total) {
    return TPercents(value, total);
}




// template impl

template <typename T>
TOutputStream& operator << (TOutputStream& o, const TShortCount<T>& f) {
    T abscount = (f.Count > 0) ? f.Count : (-1 * f.Count);

    if (abscount < f.KBase)
        return o << Simple(f.Count);

    T kbase2 = f.KBase * f.KBase;
    if (abscount < kbase2)
        return o << Simple(f.Count / (double)f.KBase) << 'K';
    else
        return o << Simple(f.Count / (double)kbase2) << 'M';
}



} // namespace NFormat
