#pragma once

#include <util/generic/stroka.h>

/// For simple cases like file name construction.
class StroCat : public Stroka {
public:
    template <class T1, class T2>
    StroCat(const T1& t1, const T2& t2)
        : Stroka(TFixedString(t1), TFixedString(t2))
    {
    }

    template <class T1, class T2, class T3>
    StroCat(const T1& t1, const T2& t2, const T3& t3)
        : Stroka(TFixedString(t1), TFixedString(t2), TFixedString(t3))
    {
    }
};

template <class... T>
inline Stroka StrCat(const T&... ts) {
    return Stroka::Join(ts...);
}
