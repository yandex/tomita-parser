#pragma once

#include "grammarhelper.h"

namespace NGleiche {

typedef TGramBitSet (*TGleicheFunc)(const TGramBitSet&, const TGramBitSet&);

/* TGleicheFunc functions */
TGramBitSet NoCheck(const TGramBitSet& g1, const TGramBitSet& g2);
TGramBitSet CaseCheck(const TGramBitSet& g1, const TGramBitSet& g2);
TGramBitSet NumberCheck(const TGramBitSet& g1, const TGramBitSet& g2);
TGramBitSet TenseCheck(const TGramBitSet& g1, const TGramBitSet& g2);

TGramBitSet CaseNumberCheck(const TGramBitSet& g1, const TGramBitSet& g2);
// standard agreement by case + first word should be Plural
TGramBitSet CaseFirstPluralCheck(const TGramBitSet& g1, const TGramBitSet& g2);
TGramBitSet GenderNumberCheck(const TGramBitSet& g1, const TGramBitSet& g2);
TGramBitSet GenderCaseCheck(const TGramBitSet& g1, const TGramBitSet& g2);
// for 1st and 2nd Persons
TGramBitSet PersonNumberCheck(const TGramBitSet& g1, const TGramBitSet& g2);
TGramBitSet GenderNumberCaseCheck(const TGramBitSet& g1, const TGramBitSet& g2);


/* Gleiche templates

There is two groups of functions: bool Gleiche(...) and TGramBitSet GleicheGrammems(...)
The first ones return true if given grammems are consistent (agree),
the second ones return grammems common for both given grammems and agreement category.

TGrammarSource generally means any collection of TGramBitSet items with const_iterator defined:
e.g. yvector<TGramBitSet>, NSpike::TGrammarBunch, etc.
But it also could be a single TGramBitSet (via template specializations)
*/

template <typename TGrammarSource1, typename TGrammarSource2>
TGramBitSet GleicheGrammems(const TGrammarSource1& g1, const TGrammarSource2& g2, TGleicheFunc func);

template <typename TGrammarSource1, typename TGrammarSource2>
bool Gleiche(const TGrammarSource1& g1, const TGrammarSource2& g2, TGleicheFunc func);

//specialization for single TGramBitSet
template <>
TGramBitSet GleicheGrammems(const TGramBitSet& g1, const TGramBitSet& g2, TGleicheFunc func);

template <>
bool Gleiche(const TGramBitSet& g1, const TGramBitSet& g2, TGleicheFunc func);

/*
//specialization for TYandexLemma
template <>
TGramBitSet GleicheGrammems(const TYandexLemma& lemma1, const TYandexLemma& lemma2, TGleicheFunc func);

template <>
bool Gleiche(const TYandexLemma& lemma1, const TYandexLemma& lemma2, TGleicheFunc func);
*/

/* some specific variants */
template <typename TGrammarSource1, typename TGrammarSource2>
TGramBitSet GleicheGrammems(const TGramBitSet& stemGram1, const TGramBitSet& stemGram2,
                            const TGrammarSource1& flexGrams1, const TGrammarSource2& flexGrams2,
                            TGleicheFunc func);

template <typename TGrammarSource1, typename TGrammarSource2>
bool Gleiche(const TGramBitSet& stemGram1, const TGramBitSet& stemGram2,
             const TGrammarSource1& flexGrams1, const TGrammarSource2& flexGrams2,
             TGleicheFunc func);

TGramBitSet GleicheGrammems(const char* stemCharGram1, const char* stemCharGram2,
                            const char* const *flexGrams1, size_t fgCount1,
                            const char* const *flexGrams2, size_t fgCount2,
                            TGleicheFunc func);

/*
//for GleicheFunc known at compile time: Gleiche<func>(g1, g2), it is just a synonym for Gleiche(g1, g2, func)
template <TGleicheFunc func, typename TGrammarSource1, typename TGrammarSource2>
bool Gleiche(const TGrammarSource1& g1, const TGrammarSource2& g2);
*/

/* normalization means generalization of grammems in several cases to increase probability of their intersection
   e.g. gMasFem should intersect with gFeminine
*/
TGramBitSet NormalizeCases(const TGramBitSet& g);
TGramBitSet NormalizeGenders(const TGramBitSet& g);
TGramBitSet NormalizeTenses(const TGramBitSet& g);
TGramBitSet NormalizeAll(const TGramBitSet& g);

typedef TGramBitSet (*TNormalizeGrammemsFunction)(const TGramBitSet&);

//intersection of normalized grammems
inline TGramBitSet GetCommonGrammems(const TGramBitSet& g1, const TGramBitSet& g2, TNormalizeGrammemsFunction normfunc = NormalizeAll)
{
    if (normfunc != NULL)
        return normfunc(g1) & normfunc(g2);
    else
        return g1 & g2;
}

//general common grammems checkers
//with one check-group
inline TGramBitSet GeneralCheck(const TGramBitSet& g1, const TGramBitSet& g2, const TGramBitSet& group,
                                TNormalizeGrammemsFunction normfunc = NormalizeAll)
{
    return GetCommonGrammems(g1, g2, normfunc) & group;
}

//with two check-groups
inline TGramBitSet GeneralCheck(const TGramBitSet& g1, const TGramBitSet& g2, const TGramBitSet& group1, const TGramBitSet& group2,
                                TNormalizeGrammemsFunction normfunc = NormalizeAll)
{
    TGramBitSet res = GetCommonGrammems(g1, g2, normfunc);
    if (res.HasAny(group1) && res.HasAny(group2))
        return res & (group1 | group2);
    else
        return TGramBitSet();
}

/* template and inline definitions */
template <typename TGrammarSource1, typename TGrammarSource2>
TGramBitSet GleicheGrammems(const TGrammarSource1& g1, const TGrammarSource2& g2, TGleicheFunc func)
{
    // comparing all possible pairs
    TGramBitSet res;
    if (func != NULL)
        for (typename TGrammarSource1::const_iterator it1 = g1.begin(); it1 != g1.end(); ++it1)
            for (typename TGrammarSource2::const_iterator it2 = g2.begin(); it2 != g2.end(); ++it2)
                res |= func(*it1, *it2);
    return res;
}

template <typename TGrammarSource1, typename TGrammarSource2>
bool Gleiche(const TGrammarSource1& g1, const TGrammarSource2& g2, TGleicheFunc func)
{
    // comparing all possible pairs - return as fast as possible
    if (func != NULL)
        for (typename TGrammarSource1::const_iterator it1 = g1.begin(); it1 != g1.end(); ++it1)
            for (typename TGrammarSource2::const_iterator it2 = g2.begin(); it2 != g2.end(); ++it2)
                if (func(*it1, *it2).any())
                    return true;
    return false;
}

template <>
inline TGramBitSet GleicheGrammems(const TGramBitSet& g1, const TGramBitSet& g2, TGleicheFunc func)
{
    if (func == NULL)
        return TGramBitSet();
    else
        return func(g1, g2);
}

template <>
inline bool Gleiche(const TGramBitSet& g1, const TGramBitSet& g2, TGleicheFunc func)
{
    return GleicheGrammems(g1, g2, func).any();
}

/*
template <>
inline TGramBitSet GleicheGrammems(const TYandexLemma& lemma1, const TYandexLemma& lemma2, TGleicheFunc func)
{
    return GleicheGrammems(lemma1.GetStemGram(), lemma2.GetStemGram(),
                           lemma1.GetFlexGram(), lemma1.FlexGramNum(),
                           lemma2.GetFlexGram(), lemma2.FlexGramNum(),
                           func);
}

template <>
inline bool Gleiche(const TYandexLemma& lemma1, const TYandexLemma& lemma2, TGleicheFunc func)
{
    return GleicheGrammems(lemma1, lemma2, func).any();
}
*/

template <typename TGrammarSource1, typename TGrammarSource2>
TGramBitSet GleicheGrammems(const TGramBitSet& stemGram1, const TGramBitSet& stemGram2,
                            const TGrammarSource1& flexGrams1, const TGrammarSource2& flexGrams2,
                            TGleicheFunc func)
{
    TGramBitSet res;
    if (func != NULL)
        for (typename TGrammarSource1::const_iterator it1 = flexGrams1.begin(); it1 != flexGrams1.end(); ++it1)
            for (typename TGrammarSource2::const_iterator it2 = flexGrams2.begin(); it2 != flexGrams2.end(); ++it2)
                res |= func(*it1 | stemGram1, *it2 | stemGram2);
    return res;
}

template <typename TGrammarSource1, typename TGrammarSource2>
bool Gleiche(const TGramBitSet& stemGram1, const TGramBitSet& stemGram2,
             const TGrammarSource1& flexGrams1, const TGrammarSource2& flexGrams2,
             TGleicheFunc func)
{
    TGramBitSet res;
    if (func != NULL)
        for (typename TGrammarSource1::const_iterator it1 = flexGrams1.begin(); it1 != flexGrams1.end(); ++it1)
            for (typename TGrammarSource2::const_iterator it2 = flexGrams2.begin(); it2 != flexGrams2.end(); ++it2)
                if (func(*it1 | stemGram1, *it2 | stemGram2).any())
                    return true;
    return false;
}

/*
template <TGleicheFunc func, typename TGrammarSource1, typename TGrammarSource2>
inline bool Gleiche(const TGrammarSource1& g1, const TGrammarSource2& g2)
{
    return Gleiche(g1, g2, func);
}
*/

}       //end of namespace NGleiche
