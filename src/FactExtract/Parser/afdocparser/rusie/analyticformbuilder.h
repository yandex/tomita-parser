#pragma once

#include <util/generic/vector.h>

#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>

struct SAnalyticalFormVariant
{
    enum eAnalyticType { Infinitive, Predikative, Short_Form, Comp_Adj };
    //word number
    int iWordNum;
    //number of homonyms for this word
    int iHomCount;
    //POS of homonym which is potential predicates
    eAnalyticType ePos;
    //lemma of verb
    Wtroka s_Lemma;
    //if all word's homonyms are predicates
    bool bAllHomPredik;
    SAnalyticalFormVariant(int iWrd, int iCount, eAnalyticType iPos, Wtroka s_lem, bool bAllPredik)
        : iWordNum(iWrd), iHomCount(iCount), ePos(iPos), s_Lemma(s_lem), bAllHomPredik(bAllPredik)
    {
    }

    DECLARE_STATIC_NAMES(TBezlPredic, "СВЕТАТЬ ТЕМНЕТЬ ХОЛОДАТЬ ТЕПЛЕТЬ СМЕРКАТЬСЯ");

    //define ordering: the best potential predicate has minimal number of homonyms
    // last in vector, and being a comparative adjective
    bool operator<(const SAnalyticalFormVariant& _X) const
    {
        if (bAllHomPredik != _X.bAllHomPredik)
            return bAllHomPredik < _X.bAllHomPredik;

        if (s_Lemma != _X.s_Lemma) {
            if (Infinitive == ePos && TBezlPredic::Has(s_Lemma))
                return true;
            if (Infinitive == ePos && TBezlPredic::Has(_X.s_Lemma))
                return false;
        }

        if (iHomCount < _X.iHomCount && Comp_Adj != ePos && Infinitive != ePos)
            return true;

        if (Comp_Adj != ePos && Infinitive != ePos &&
             (Comp_Adj == _X.ePos || Infinitive == _X.ePos))
            return true;

        if (Infinitive == ePos && Comp_Adj == _X.ePos)
            return true;

        return false;

    };
};

typedef yvector<SAnalyticalFormVariant> CAnalyticalFormVars;

class CAnalyticFormBuilder
{
public:
    CAnalyticFormBuilder(CWordVector&  Words, yvector<SWordHomonymNum>& clauseWords);
    void Run();

protected:
    CWordVector&   m_Words;
    yvector<SWordHomonymNum>& m_ClauseWords;

protected:
    bool HasAnalyticalBe(const CWord& _W) const;
    bool HasDeclinableSynNounInInstrumentalis(const CWord& _W) const;
    bool CheckComparativeZAPLATAForAnalyticalForm (long lWordNo) const;
    bool HasShortParticipleOrAdj(const CWord& _W) const;
    void BuildAnalyticalVerbFormsZaplata2();
    bool CheckAnalyticalVerbForm(int iVWrd, int iSWrd);
    bool IsAnalyticalVerbForm(int iVerbWrd, int iSWrd, int& VerbHomNo, yvector<int>& AnalyticHom, bool& b_AddAuxVerbHom);
    void ChangeGrammemsAsAnalyticForm(CHomonym& H, const CHomonym& VerbHomonym);
    bool HasCompar(const CWord& W);
    bool HasPredik(const CWord& W);
    bool HasInfinitive(const CWord& W);
    bool AllHomonymsArePredicates(const CWord& W) const;
    void BuildAnalyticalVerbForms();
};
