#include "gleiche.h"

namespace NGleiche {

TGramBitSet GleicheGrammems(const char* stemCharGram1, const char* stemCharGram2,
                            const char* const *flexGrams1, size_t fgCount1,
                            const char* const *flexGrams2, size_t fgCount2,
                            TGleicheFunc func)
{
    if (!stemCharGram1 || !stemCharGram2)
        return TGramBitSet();

    NSpike::TGrammarBunch g1, g2;
    NSpike::ToGrammarBunch(stemCharGram1, flexGrams1, fgCount1, g1);
    NSpike::ToGrammarBunch(stemCharGram2, flexGrams2, fgCount2, g2);

    return GleicheGrammems(g1, g2, func);
}

//считаем, что при согласовании нужно приравнивать
//2-й родительный к родительному
//2-й предложный к предложному
//звательный к именительному
TGramBitSet NormalizeCases(const TGramBitSet& g)
{
    TGramBitSet res = g;

    if (g.Has(gPartitive))
        res.Set(gGenitive);

    if (g.Has(gLocative))
        res.Set(gAblative);

    if (g.Has(gVocative))
        res.Set(gNominative);

    return res;
}

TGramBitSet NormalizeGenders(const TGramBitSet& g)
{
    TGramBitSet res = g;
    if (g.Has(gMasFem))
    {
        res.Set(gMasculine);
        res.Set(gFeminine);
    }

    return res;
}

TGramBitSet NormalizeTenses(const TGramBitSet& g)
{
    TGramBitSet res = g;
    if (res.Has(gNotpast))
    {
        res.Reset(gNotpast);
        if (res.Has(gImperfect))
            res.Set(gPresent);
        if (res.Has(gPerfect))
            res.Set(gFuture);
    }
    return res;
}

TGramBitSet NormalizeAll(const TGramBitSet& g)
{
    //activation mask
    static const TGramBitSet mask = TGramBitSet(gMasFem, gPartitive, gLocative, gVocative) |
                                    TGramBitSet(gNotpast /*, gImperfect, gPerfect*/);
    if (g.HasAny(mask))
        return NormalizeTenses(NormalizeCases(NormalizeGenders(g)));
    else
        return g;
}

/* TGleicheFunc definitions */

TGramBitSet NoCheck(const TGramBitSet&, const TGramBitSet&)
{
    return NSpike::AllGrammems;
}

TGramBitSet CaseCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    return GeneralCheck(g1, g2, NSpike::AllCases, NormalizeCases);
}

TGramBitSet NumberCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    return GeneralCheck(g1, g2, NSpike::AllNumbers, NULL);
}

TGramBitSet TenseCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    return GeneralCheck(g1, g2, NSpike::AllTimes, NormalizeTenses);
}

TGramBitSet CaseNumberCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    return GeneralCheck(g1, g2, NSpike::AllCases, NSpike::AllNumbers, NormalizeCases);
}

TGramBitSet CaseFirstPluralCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    if (g1.Has(gPlural))
        return CaseCheck(g1, g2);
    else
        return TGramBitSet();
}

TGramBitSet GenderNumberCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    TGramBitSet res = GetCommonGrammems(g1, g2, NormalizeGenders);
    //additionally consider gender the same if both words are Plural
    if (res.Has(gPlural) || (res.Has(gSingular) && res.HasAny(NSpike::AllGenders)))
        return res & (NSpike::AllNumbers | NSpike::AllGenders);
    else
        return TGramBitSet();
}

TGramBitSet GenderCaseCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    return GeneralCheck(g1, g2, NSpike::AllGenders, NSpike::AllCases, NormalizeAll);
}

TGramBitSet PersonNumberCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    return GeneralCheck(g1, g2, NSpike::AllPersons, NSpike::AllNumbers, NULL);
}

TGramBitSet GenderNumberCaseCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    TGramBitSet res = GetCommonGrammems(g1, g2, NormalizeAll);

    if (res.HasAny(NSpike::AllCases) && res.HasAny(NSpike::AllNumbers) &&
        (res.HasAny(NSpike::AllGenders) || !g1.HasAny(NSpike::AllGenders) || !g2.HasAny(NSpike::AllGenders)))
    {
        static const TGramBitSet all_gnc = NSpike::AllNumbers | NSpike::AllCases | NSpike::AllGenders;
        res &= all_gnc;
        //check special corner-case - singular accusative masculine
        static const TGramBitSet spec(gSingular, gMasculine, gAccusative);
        if (res.HasAll(spec))
        {
            //if one is definitely animated, and other is definitely inaminated - cancel such agreement
            //examples [tranlit]: vozglavivshego bank, dom druga
            bool a1 = g1.Has(gAnimated), a2 = g2.Has(gAnimated);
            if (a1 != g1.Has(gInanimated) && a2 != g2.Has(gInanimated) && a1 != a2)
                res.Reset();
        }
        return res;
    }
    return TGramBitSet();
}

}  //end of namespace NGleiche
