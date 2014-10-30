#pragma once

#include <library/lemmer/dictlib/grammarhelper.h>
#include <library/lemmer/dictlib/gleiche.h>
#include <FactExtract/Parser/common/common_constants.h>


class CHomonym;
class THomonymGrammems;

namespace NGleiche {

// some specific agreement procedures

//согласование между подлежащим и сказуемым (предикатом - это может быть глагол или краткое прилагательное/причастие)
TGramBitSet SubjectPredicateCheck(const TGramBitSet& gsubj, const TGramBitSet& gpredic);

//феминистическое cогласование между двумя именами  по  падежу и роду для примера
// 'поэт Ахматова'
// *'поэтесса Маяковский'
// доверенное лицо министра Ахматова
// доверенное лицо министра Ахматов
//т.е. если род g1 = g2 или (g1 = мр. и g2 = жр. )
TGramBitSet FeminCaseCheck(const TGramBitSet& g1, const TGramBitSet& g2);

// Согласование по изафету (исходно: для турецкого языка)
TGramBitSet IzafetCheck(const TGramBitSet& g1, const TGramBitSet& g2);

// Согласование слов после числительного в русском (напр, "(два) бывших мужа")
TGramBitSet AfterNumberCheck(const TGramBitSet& g1, const TGramBitSet& g2);

//согласует по числу и лицу/роду
bool GleicheSimPredics(const CHomonym& h1, const CHomonym& h2);

using NSpike::TGrammarBunch;

//specialization for THomonymGrammems
template <> bool Gleiche(const THomonymGrammems& h1, const THomonymGrammems& h2, TGleicheFunc func);
template <> TGramBitSet GleicheGrammems(const THomonymGrammems& h1, const THomonymGrammems& h2, TGleicheFunc func);

//specialization for CHomonym
template <> bool Gleiche(const CHomonym& h1, const CHomonym& h2, TGleicheFunc func);
template <> TGramBitSet GleicheGrammems(const CHomonym& h1, const CHomonym& h2, TGleicheFunc func);

/* ECoordFunc templates */

//ECoordFunc to TGleicheFunc convertor
template <ECoordFunc coord_func>
inline TGleicheFunc GetGleicheFunc() { return NULL; }        // by default

//static mapping ECoordFunc -> TGleicheFunc (for currently defined variants, should be extended when more TGleicheFunc's are defined)
template <> inline TGleicheFunc GetGleicheFunc<SubjVerb>()         { return NGleiche::SubjectPredicateCheck; }
template <> inline TGleicheFunc GetGleicheFunc<SubjAdjShort>()     { return NGleiche::SubjectPredicateCheck; }
template <> inline TGleicheFunc GetGleicheFunc<GendreNumberCase>() { return NGleiche::GenderNumberCaseCheck; }
template <> inline TGleicheFunc GetGleicheFunc<GendreNumber>()  { return NGleiche::GenderNumberCheck; }
template <> inline TGleicheFunc GetGleicheFunc<GendreCase>()    { return NGleiche::GenderCaseCheck; }
template <> inline TGleicheFunc GetGleicheFunc<NumberCaseAgr>() { return NGleiche::CaseNumberCheck; }
template <> inline TGleicheFunc GetGleicheFunc<CaseAgr>()       { return NGleiche::CaseCheck; }
template <> inline TGleicheFunc GetGleicheFunc<FeminCaseAgr>()  { return NGleiche::FeminCaseCheck; }
template <> inline TGleicheFunc GetGleicheFunc<NumberAgr>()     { return NGleiche::NumberCheck; }
template <> inline TGleicheFunc GetGleicheFunc<IzafetAgr>()     { return NGleiche::IzafetCheck; }
template <> inline TGleicheFunc GetGleicheFunc<AfterNumAgr>()   { return NGleiche::AfterNumberCheck; }
template <> inline TGleicheFunc GetGleicheFunc<AnyFunc>()       { return NGleiche::NoCheck; }

//dynamic mapping ECoordFunc -> TGleicheFunc
inline TGleicheFunc GetGleicheFunc(ECoordFunc func)
{
    switch (func) {
        case SubjVerb:          return NGleiche::SubjectPredicateCheck;
        case SubjAdjShort:      return NGleiche::SubjectPredicateCheck;
        case GendreNumberCase:  return NGleiche::GenderNumberCaseCheck;
        case GendreNumber:      return NGleiche::GenderNumberCheck;
        case GendreCase:        return NGleiche::GenderCaseCheck;
        case NumberCaseAgr:     return NGleiche::CaseNumberCheck;
        case CaseAgr:           return NGleiche::CaseCheck;
        case FeminCaseAgr:      return NGleiche::FeminCaseCheck;
        case NumberAgr:         return NGleiche::NumberCheck;
        case IzafetAgr:         return NGleiche::IzafetCheck;
        case AfterNumAgr:       return NGleiche::AfterNumberCheck;
        case AnyFunc:           return NGleiche::NoCheck;

        default:                return NULL;        // others (if any) are not defined yet - current stub would return false
    }
}

template <typename TGrammarSource1, typename TGrammarSource2>
inline bool Gleiche(const TGrammarSource1& g1, const TGrammarSource2& g2, ECoordFunc func)
{
    return Gleiche(g1, g2, GetGleicheFunc(func));
}

template <typename TGrammarSource1, typename TGrammarSource2>
inline TGramBitSet GleicheGrammems(const TGrammarSource1& g1, const TGrammarSource2& g2, ECoordFunc func)
{
    return GleicheGrammems(g1, g2, GetGleicheFunc(func));
}

//generics for ECoordFunc known at compile time
template <ECoordFunc func, typename TGrammarSource1, typename TGrammarSource2>
inline bool Gleiche(const TGrammarSource1& g1, const TGrammarSource2& g2)
{
    return Gleiche(g1, g2, GetGleicheFunc<func>());
}

template <ECoordFunc func, typename TGrammarSource1, typename TGrammarSource2>
inline TGramBitSet GleicheGrammems(const TGrammarSource1& g1, const TGrammarSource2& g2)
{
    return GleicheGrammems(g1, g2, GetGleicheFunc<func>());
}

// additionally returns compatible grammems from each of input grammems set.
TGramBitSet GleicheForms(const TGrammarBunch& grams1, const TGrammarBunch& grams2,
                         TGrammarBunch& res_grams1, TGrammarBunch& res_grams2,
                         ECoordFunc func);

//variations
TGramBitSet GleicheForms(const TGramBitSet& grams1, const TGrammarBunch& grams2, TGrammarBunch& res_grams2, ECoordFunc func);

//restores (separates) forms of grams2 using information about agreements with forms of grams1
//useful when grammems forms info is mixed into single grammems-set
TGramBitSet GleicheAndRestoreForms(const TGrammarBunch& grams1, const TGramBitSet& grams2,
                                   TGrammarBunch& res_grams1, TGrammarBunch& res_grams2,
                                   ECoordFunc func);

}       // end of namespace NGleiche
