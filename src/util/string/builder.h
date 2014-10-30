#pragma once

#include <util/stream/str.h>
#include <util/generic/stroka.h>

class TStringBuilder: public Stroka {
public:
    inline TStringBuilder()
        : Out(*this)
    {
    }

    template <class T>
    inline TStringBuilder& operator<< (const T& t) {
        Out << t;

        return *this;
    }

private:
    TStringOutput Out;
};
