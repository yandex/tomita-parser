#include "gleiche.h"
#include <FactExtract/Parser/afdocparser/rus/morph.h>
#include <FactExtract/Parser/afdocparser/rus/homonym.h>

namespace NGleiche {

template <>
bool Gleiche(const THomonymGrammems& g1, const THomonymGrammems& g2, TGleicheFunc func)
{
    //first look at Forms
    if (g1.HasForms() && g2.HasForms())
        return Gleiche(g1.Forms(), g2.Forms(), func);
    else
        return Gleiche(g1.All(), g2.All(), func);
}

template <>
TGramBitSet GleicheGrammems(const THomonymGrammems& g1, const THomonymGrammems& g2, TGleicheFunc func)
{
    if (g1.HasForms() && g2.HasForms())
        return GleicheGrammems(g1.Forms(), g2.Forms(), func);
    else
        return GleicheGrammems(g1.All(), g2.All(), func);
}

template <>
bool Gleiche(const CHomonym& h1, const CHomonym& h2, TGleicheFunc func) {
    return Gleiche(h1.Grammems, h2.Grammems, func);
}

template <>
TGramBitSet GleicheGrammems(const CHomonym& h1, const CHomonym& h2, TGleicheFunc func) {
    return GleicheGrammems(h1.Grammems, h2.Grammems, func);
}

TGramBitSet GleicheForms(const TGrammarBunch& grams1, const TGrammarBunch& grams2,
                         TGrammarBunch& res_grams1, TGrammarBunch& res_grams2,
                         ECoordFunc func)
{
    TGramBitSet res;
    TGleicheFunc gfunc = GetGleicheFunc(func);
    if (gfunc != NULL)
        for (TGrammarBunch::const_iterator it1 = grams1.begin(); it1 != grams1.end(); ++it1)
            for (TGrammarBunch::const_iterator it2 = grams2.begin(); it2 != grams2.end(); ++it2) {
                TGramBitSet tmp = gfunc(*it1, *it2);
                if (tmp.any()) {
                    res_grams1.insert(*it1);
                    res_grams2.insert(*it2);
                    res |= tmp;
                }
            }
    return res;
};

TGramBitSet GleicheForms(const TGramBitSet& grams1, const TGrammarBunch& grams2, TGrammarBunch& res_grams2, ECoordFunc func)
{
    TGrammarBunch tmp, res_tmp;
    tmp.insert(grams1);
    return GleicheForms(tmp, grams2, res_tmp, res_grams2, func);
}

TGramBitSet GleicheAndRestoreForms(const TGrammarBunch& grams1, const TGramBitSet& grams2,
                                   TGrammarBunch& res_grams1, TGrammarBunch& res_grams2,
                                   ECoordFunc func)
{
    TGramBitSet res;
    TGleicheFunc gfunc = GetGleicheFunc(func);
    if (gfunc != NULL)
        for (TGrammarBunch::const_iterator it1 = grams1.begin(); it1 != grams1.end(); ++it1) {
            TGramBitSet tmp = gfunc(*it1, grams2);
            if (tmp.any()) {
                res_grams1.insert(*it1);

                //try to restore an agreed form grams2 (which are mixed forms)
                TGramBitSet res2_item = grams2;
                res2_item.ReplaceByMaskIfAny(tmp, NSpike::AllCases);
                res2_item.ReplaceByMaskIfAny(tmp, NSpike::AllGenders);
                res2_item.ReplaceByMaskIfAny(tmp, NSpike::AllNumbers);
                res2_item.ReplaceByMaskIfAny(tmp, NSpike::AllTimes);
                res2_item.ReplaceByMaskIfAny(tmp, NSpike::AllPersons);
                static const TGramBitSet all_anim(gAnimated, gInanimated);
                res2_item.ReplaceByMaskIfAny(tmp, all_anim);
                res_grams2.insert(res2_item);

                res |= tmp;
            }
        }
    return res;
}

TGramBitSet SubjectPredicateCheck(const TGramBitSet& gsubj, const TGramBitSet& gpredic)
{
    if (!gsubj.Has(gNominative))
        return TGramBitSet();

    TGramBitSet both = GetCommonGrammems(gsubj, gpredic, NormalizeTenses);

    if (TMorph::IsPastTense(gpredic) || gpredic.Has(gShort)) {
        // ты вышел
        // я вышел
        // ты был
        // я красива
        // мы шли
        // вы шли
        if (gsubj.Has(gPerson1) || gsubj.Has(gPerson2)) {
            both |= gsubj & NSpike::AllPersons;
            if (both.Has(gPlural))
                return both;
            else if (both.Has(gSingular) && (gpredic.Has(gMasculine) || gpredic.Has(gFeminine)))
                return both | (gpredic & TGramBitSet(gMasculine, gFeminine));
            else
                return TGramBitSet();
        } else
            // он вышел
            // поезд ушел
            // девочка красива
            // девочки красивы
            // мальчик красив
            return GenderNumberCheck(gsubj, gpredic);
    } else if (TMorph::IsPresentOrFutureTense(gpredic)) {
        //  я выйду
        //  ты выедешь
        //  мы выйдем
        if (both.Has(gPerson1) || both.Has(gPerson2))
            return PersonNumberCheck(gsubj, gpredic);
        else
            return NumberCheck(gsubj, gpredic);
    } else if (gpredic.Has(gImperative)) {
        TGramBitSet res = NumberCheck(gsubj, gpredic);
        if (res.none() || !gsubj.Has(gPerson2))
            return TGramBitSet();
        res.Set(gPerson2);
        return res;
    } else
        return TGramBitSet();

}

TGramBitSet FeminCaseCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    TGramBitSet status = g1, name = g2;
    if (g1.HasAny(TGramBitSet(gSurname, gFirstName))) {
        name = g1;
        status = g2;
    }
    TGramBitSet res = GetCommonGrammems(name, status, NormalizeAll);

    if (res.HasAny(NSpike::AllCases) && res.HasAny(NSpike::AllNumbers)) {
        res &= (NSpike::AllCases | NSpike::AllNumbers | NSpike::AllGenders);    //common grammems
        if (res.HasAny(NSpike::AllGenders))
            return res;
        else if (name.Has(gFeminine)) // "поэт Ахматова" или "физлицо Цветаева"
            return res | TGramBitSet(gFeminine);
        else if (name.Has(gMasculine) && !status.Has(gFeminine)) // "поэт Пушкин" или "физлицо Лермонтов", но не "поэтесса Пушкин"
            return res | TGramBitSet(gMasculine);
    }
    return TGramBitSet();
}

TGramBitSet IzafetCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    static const TGramBitSet izafet = TGramBitSet(gPoss1pSg, gPoss1pPl, gPoss2pSg, gPoss2pPl) |
                                      TGramBitSet(gPoss3p, gPoss3pSg, gPoss3pPl);
    if (g1.Has(gSubstantive) && g1.HasAny(TGramBitSet(gNominative, gGenitive)) &&
        g2.Has(gSubstantive) && g2.HasAny(izafet))
        return g2;
    else
        return TGramBitSet();
}

static inline bool AfterNumberCheck(const TGramBitSet& g)
{
    if (!g.Has(gGenitive))
        return false;
    if (TMorph::IsMorphAdjective(g))
        return g.Has(gPlural);              // американских
    else
        return TMorph::IsMorphNoun(g);     // 2 президента, 5 президентов
}

TGramBitSet AfterNumberCheck(const TGramBitSet& g1, const TGramBitSet& g2)
{
    if (AfterNumberCheck(g1) && AfterNumberCheck(g2))
        return TGramBitSet(gPlural);
    else
        return GenderNumberCaseCheck(g1, g2);
}

bool GleicheSimPredics(const CHomonym& h1, const CHomonym& h2)
{
    TGramBitSet res = GetCommonGrammems(h1.Grammems.All(), h2.Grammems.All(), NormalizeAll);

    if (!res.HasAny(NSpike::AllNumbers))
        return false;

    bool p1 = h1.IsShortAdjectiveOrParticiple();
    bool p2 = h2.IsShortAdjectiveOrParticiple();

    if (p1 && p2)
        return h1.HasGrammem(gPlural) || res.HasAny(NSpike::AllGenders);

    if (p1 || p2)
        return true;

    return res.HasAny(NSpike::AllGenders) ||
           res.HasAny(NSpike::AllPersons) ||
           h1.IsPastTense() || h2.IsPastTense();
}

} //end of namespace NGleiche
