#include "dictutil.h"

template<typename TCharType>
void PrefSuffT(const TCharType* s, int lenS, const TCharType* t, int lenT,
    int* prefix, int* suffix, EPrefSuffMode mode)
{
    int minLen = Min(lenS, lenT);

    int pref = 0;
    while (pref < minLen && s[pref] == t[pref])
        ++pref;

    const TCharType* pS = s + lenS - 1; // The last symbol.
    const TCharType* pT = t + lenT - 1; // -------"-------
    int suff = 0;
    while (suff < minLen && pS[-suff] == pT[-suff])
        ++suff;

    *prefix = pref;
    *suffix = suff;
    if (!(mode & PS_GREEDY_PREFIX))
        *prefix = Min(pref, minLen - suff);
    if (!(mode & PS_GREEDY_SUFFIX))
        *suffix = Min(suff, minLen - pref);
}

////////////////////////////////////////////////////////////////////////////////
//
// Functions
//

void PrefSuff(const wchar16* s, int lenS, const wchar16* t, int lenT,
    int* prefix, int* suffix, EPrefSuffMode mode)
{
    PrefSuffT(s, lenS, t, lenT, prefix, suffix, mode);
}

void PrefSuff(const char* s, int lenS, const char* t, int lenT,
    int* prefix, int* suffix, EPrefSuffMode mode)
{
    PrefSuffT(s, lenS, t, lenT, prefix, suffix, mode);
}
