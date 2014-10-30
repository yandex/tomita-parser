#pragma once

#include <util/generic/stroka.h>

////////////////////////////////////////////////////////////////////////////////
//
// StrMetric
//

template<class TStroka>
class TStrMetric {
    DECLARE_NOCOPY(TStrMetric);

protected:
    TStrMetric() {}
    virtual ~TStrMetric() {}

public:
    typedef typename TStroka::char_type TCharType;
    int Dist(const TStroka& from, const TStroka& to) {
        return DoDist(
            from.c_str(), (int)from.length(), to.c_str(), (int)to.length());
    }

    int Dist(const TCharType* from, const TCharType* to) {
        return DoDist(from, (int)TStroka::StrLen(from), to, (int)TStroka::StrLen(to));
    }

    int Dist(const TCharType* from, size_t fromLen, const TCharType* to, size_t toLen) {
        return DoDist(from, int(fromLen), to, int(toLen));
    }

protected:
    virtual int DoDist(const TCharType* s, int lenS, const TCharType* t, int lenT) = 0;
};

extern TStrMetric<Stroka>& Levenshtein;
extern TStrMetric<Stroka>& Damerau;

////////////////////////////////////////////////////////////////////////////////
//
// Functions
//

// Quick string-distance calculation.
// @return value which is less or equal to Levenshtein distance between s1 and s2
int QuickStrDist(const char* s1, int len1, const char* s2, int len2);

inline int QuickStrDist(const Stroka& from, const Stroka& to) {
    return QuickStrDist(~from, (int)from.length(), ~to, (int)to.length());
}
