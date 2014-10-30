#include "str_metric.h"
#include "dictutil.h"
#include <util/charset/codepage.h>
#include <util/draft/matrix.h>
#include <util/memory/tempbuf.h>

////////////////////////////////////////////////////////////////////////////////
//
// EditDist
//

template<bool TR, class TStroka>
class TEditDist : public TStrMetric<TStroka> {
    DECLARE_NOCOPY(TEditDist);

public:
    TEditDist() {}
    typedef typename TStroka::char_type TCharType;

protected:
    virtual int DoDist(const TCharType* s, int lenS, const TCharType* t, int lenT);

    void FillMatrix(TMatrix<int>& d,
        int m, int n, const TCharType* s, const TCharType* t);
};

template<bool TR, class TStroka>
int TEditDist<TR, TStroka>::DoDist(
    const TCharType* s, int lenS, const TCharType* t, int lenT)
{
    // Find common prefix/suffix
    int pref = 0, suff = 0;
    PrefSuff(s, lenS, t, lenT, &pref, &suff);
    s += pref; int m = lenS - pref - suff;
    t += pref; int n = lenT - pref - suff;

    if (m == 0)
        return n;
    if (n == 0)
        return m;

    TTempBuf buf((m + 1) * (n + 1) * sizeof(int));
    TMatrix<int> d(m + 1, n + 1, (int*)buf.Data());

    // Calculate distance
    FillMatrix(d, m, n, s, t);
    return d[m][n];
}

#ifdef _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant
#endif // MS VC++

template<bool TR, class TStroka>
void TEditDist<TR, TStroka>::FillMatrix(
    TMatrix<int>& d, int m, int n, const TCharType* s, const TCharType* t)
{
    const TCharType* s1 = s - 1; // 1-based
    const TCharType* t1 = t - 1; // 1-based

    d[0][0] = 0;

    int* row = d[0]; // points to 1st row
    for (int j = 1; j <= n; ++j)
        row[j] = j;

    for (int i = 1; i <= m; ++i)
        d[i][0] = i;

    row = d[0]; // points to 1st row
    for (int i = 1; i <= m; ++i) {
        int* prev = row; row = d[i];
        for (int j = 1; j <= n; ++j) {
            int substCost = (s1[i] == t1[j] ? 0 : 1);
            row[j] = min3(row[j - 1] + 1, prev[j] + 1, prev[j - 1] + substCost);
            if (TR && i > 1 && j > 1 && s1[i - 1] == t1[j] && s1[i] == t1[j - 1])
                row[j] = Min(row[j], d[i - 2][j - 2] + 1);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// DamerauDist
//

typedef TEditDist<true, Stroka> TDamerauDist;

////////////////////////////////////////////////////////////////////////////////
//
// LevenshteinDist
//

class TLevenshteinDist : public TEditDist<false, Stroka> {
    DECLARE_NOCOPY(TLevenshteinDist);

// Constructor
public:
    TLevenshteinDist()
      : TEditDist<false, Stroka>()
    {}

public:
    correction_mask DiffMask(const Stroka& from, const Stroka& to) {
        return DoDiffMask(
            from.c_str(), (int)from.length(), to.c_str(), (int)to.length());
    }

    correction_mask DiffMask(const char* from, const char* to) {
        return DoDiffMask(from, (int)strlen(from), to, (int)strlen(to));
    }

private:
    correction_mask DoDiffMask(
        const char* strS, int lenS, const char* strT, int lenT);
};

correction_mask TLevenshteinDist::DoDiffMask(
    const char* s, int lenS, const char* t, int lenT)
{
    int pref = 0, suff = 0;
    PrefSuff(s, lenS, t, lenT, &pref, &suff);
    if (suff > 0)
        --suff; // include equal characters in Matrix
    s += pref; int m = lenS - pref - suff;
    t += pref; int n = lenT - pref - suff;

    TMatrix<int> d(m + 1, n + 1);

    // Calculate distance
    FillMatrix(d, m, n, s, t);

    std::vector<char> mask(n + 2);

    int i = m, j = n;
    const char *s1 = s - 1, *t1 = t - 1; // 1-based
    while (i >= 0 && j >= 0) {
        const int* row = d[i];
        const int* prev = (i > 0) ? d[i - 1] : 0;
        int v = row[j];
        char chS = (i + pref > 0) ? s1[i] : (char)0;
        char chT = (j + pref > 0) ? t1[j] : (char)0;
        bool eq = chS == chT;

        if (i > 0 && j > 0 && prev[j - 1] < v) { // substitution
            if (!csYandex.IsSpace(chT))
                mask[j] = 1;
            --i; --j;
        }
        else if (j > 0 && row[j - 1] < v) { // insert
            if (!csYandex.IsSpace(chT)) {
                mask[j] = 1;
                if (eq) // duplication
                    mask[j - 1] = 1;
            }
            --j;
        }
        else if (i > 0 && prev[j] < v) { // delete
            if (!csYandex.IsSpace(chS) && !csYandex.IsPunct(chS)) {
                if (s1[i - 1] != t1[j] || !mask[j + 1]) // for "рассказы" -> "разказы"
                    mask[j] = 1;
                if (!eq)
                    mask[j + 1] = 1;
            }
            --i;
        }
        else { // equal
            --i; --j;
        }
    }

    correction_mask result = 0;
    int pos = pref - 1;
    int pos_max = Min(lenT, (int)sizeof(result) * 8) - 1;
    for (i = 0; i <= n + 1 && pos <= pos_max; ++i, ++pos) {
        if (pos >= 0 && mask[i])
            result |= (1ULL << pos);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
//
// Instances
//

static TLevenshteinDist LevenshteinInstance;
static TDamerauDist     DamerauInstance;
static TEditDist<false, WStroka> WideDistInstance;

TStrMetric<Stroka>& Levenshtein(LevenshteinInstance);
TStrMetric<Stroka>& Damerau(DamerauInstance);

////////////////////////////////////////////////////////////////////////////////
//
// Functions
//

Stroka markup_diff(
    const Stroka& source, const Stroka& target,
    const Stroka& open_tag, const Stroka& close_tag, int flags)
{
    correction_mask mask = diffmask(
        MapString(LANG_RUS, flags, source),
        MapString(LANG_RUS, flags, target));

    size_t target_len = target.length();
    size_t mask_size = sizeof(correction_mask) * 8;
    bool flag = false;
    Stroka result;

    for (size_t i = 0; i <= target_len; ++i) {
        bool new_flag = false;
        if (i < mask_size && i < target_len)
            new_flag = (mask & (1ULL << i)) != 0;
        if (new_flag != flag)
            result.append(new_flag ? open_tag : close_tag);
        if (i < target_len)
            result.append(target[i]);
        flag = new_flag;
    }

    return result;
}

correction_mask diffmask(const Stroka& left, const Stroka& right) {
    return LevenshteinInstance.DiffMask(left, right);
}

int strdist(const char* from, const char* to) {
    return LevenshteinInstance.Dist(from, to);
}

int strdist(const Stroka& from, const Stroka& to) {
    return LevenshteinInstance.Dist(from, to);
}

int strdist(const wchar16* from, const wchar16* to) {
    return WideDistInstance.Dist(from, to);
}

int strdist(const WStroka& from, const WStroka& to) {
    return WideDistInstance.Dist(from, to);
}

int QuickStrDist(const char* s1, int len1, const char* s2, int len2) {
    ui32 mask1[8] = {}, mask2[8] = {};

    for (int i = 0; i != len1; ++i) {
        unsigned char b = (unsigned char) s1[i];
        mask1[b / 32] |= 1 << (b % 32);
    }

    for (int i = 0; i != len2; ++i) {
        unsigned char b = (unsigned char) s2[i];
        mask2[b / 32] |= 1 << (b % 32);
    }

    int dist1 = 0, dist2 = 0;
    for (int i = 1; i < 8; ++i) { // start from 1 because mask1[0] == mask2[0] == 0.
        if (mask1[i] == mask2[i])
            continue;

        dist1 += CountBits32(mask1[i] & ~mask2[i]);
        dist2 += CountBits32(mask2[i] & ~mask1[i]);
    }

    if (len1 != len2) {
        if (len1 < len2)
            dist1 += len2 - len1;
        else
            dist2 += len1 - len2;
    }

    return Max(dist1, dist2);
}
