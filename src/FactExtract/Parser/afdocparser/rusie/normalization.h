#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>
#include <util/generic/set.h>

#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include <FactExtract/Parser/afdocparser/rus/morph.h>
#include <FactExtract/Parser/afdocparser/common/datetime.h>
#include "factgroup.h"
#include "dictsholder.h"


class CNormalization
{
public:
    CNormalization(const CWordVector& rWords, const CWorkGrammar* pGram = NULL);

    // sequence normalization
    void Normalize(CFactFields& fact, const yvector<SWordHomonymNum>& FdoChainWords, const CFactSynGroup* pGroup) const;

private:
    void NormalizeWordSequence(const yvector<SWordHomonymNum>& FDOChainWords, const CFactSynGroup* pGroup,
                               CTextWS* pWs, yset<int>& excludedWords, TGramBitSet caseForNorm = TGramBitSet(gNominative)) const;
    void NormalizeWordSequence(const yvector<SWordHomonymNum>& FDOChainWords, const CFactSynGroup* pGroup,
                               CTextWS* pWs, const TGramBitSet& caseForNorm = TGramBitSet(gNominative)) const;


    void NormalizeWordSequenceOnlyWithDictionariesLemmas(const yvector<SWordHomonymNum>& FDOChainWords,
                                                         const CFactSynGroup* pParentGroup, CTextWS* pWs) const;
public:
    // single word normalization
    Wtroka GetArtificialLemma(const SWordHomonymNum& Word, const TGramBitSet& Grammems = TGramBitSet(gNominative)) const;
    Wtroka GetFormWithGrammems(const CWord& W, const CHomonym& Hom, TGramBitSet Grammems) const;
    static Wtroka GetCapitalizedLemma(const CWord& w, const Wtroka& strLemma);


    // misc
    bool CheckCompanyShortName(const CWordSequence& wsCompName, const CWordSequence& wsCompanyShortName) const;
    int HasQuotedCompanyNameForNormalization(CWordSequence& quoted_compname) const;
    void QuotedWordNormalize(CWordSequence& ws);

    int GetFirstHomonymInTomitaGroup(const yvector<SWordHomonymNum>& FDOChainWords,
                                     const CFactSynGroup* pGroup, int WordNo) const;


private:

    int GetHomonymByTerminalWithTheseGrammems(const CWord& w, TerminalSymbolType s, const TGramBitSet& Grammems) const;

    CWordsPair ConvertToTomitaCoordinates(const yvector<SWordHomonymNum>& FDOChainWords, const CWordsPair& Group) const;

    void DateNormalize(CWordSequence* ws) const;

    void UpdateFioFactField(const CFactSynGroup* pParentGroup, const yvector<SWordHomonymNum>& FdoChainWords, CFioWS& newFioWS) const;

    void GetExcludedWords(const CGroup* pGroup, yset<int>& excludedWords)const;
    bool ShiftMainWordIfOdinIz(const yvector<SWordHomonymNum>& FDOChainWords, const CFactSynGroup* pGroup,
                               int& MainWordNo, int& HomNo, size_t& ShiftSize) const;

    bool HomonymCheckedByKWType(const CFactSynGroup* pGroup, SWordHomonymNum wh, CWordsPair wpFromFDOChainWords) const;
    void GetBranch(yvector<const CGroup*>& groups, const CWordsPair& wp, const CGroup* pCurGroup)const;
    const CFactSynGroup* GetChildGroupByWordSequence(const yvector<SWordHomonymNum>& FDOChainWords,
                                                     const CFactSynGroup* pGroup, const CWordSequence* pWs) const;


    void NormalizeSimpleWordSequence(const CWordsPair& wp, yvector<SWordSequenceLemma>& lemmas,
                                     SWordHomonymNum mainWH, const TGramBitSet& caseForNorm) const;

    Wtroka GetFormWithGrammemsAndCheck(const CWord& W, const CHomonym& Hom, const TGramBitSet& Grammems) const;
    Wtroka GetDictionaryLemma(SWordHomonymNum wh, const TGramBitSet& grammems, const CWordsPair& pWs) const;

    Wtroka GetFormWithThisGrammems(const CHomonym& hom, const Wtroka& original_form,
                                   const TGramBitSet& common_grammems, ETypeOfPrim eGrafemWrdType) const;
    Wtroka GetFormWithThisGrammems(const Wtroka& lemma, const Wtroka& original_form,
                                   const TGramBitSet& common_grammems) const;
    Wtroka GetOriginalWordStrings(const yvector<SWordHomonymNum>& FDOChainWords, int WordNo, const CFactSynGroup* pGroup,
                                  const CWordSequence& wsForNormalization, bool bOnlyWhenAlwaysReplace = false) const;

    Wtroka GetFormOfTheFirstPartOfHyphenedWord(Wtroka FirstPart, const CHomonym& hom, const TGramBitSet& common_grammems) const;

    TGramBitSet GetLemmaGrammems(const CHomonym& hom, ETypeOfPrim eGrafemWrdType) const;

    static Wtroka GetCapitalizedLemmaSimple(const CWord& w, Wtroka strLemma);
    static Wtroka GetCapitalizedLemma(const yvector<SWordHomonymNum>& FDOChainWords,
                                      SWordHomonymNum wh, const Wtroka& strLemma);

private:
    const CWordVector& m_Words;
    const CWorkGrammar* m_pGLRgrammar;  //Острожно!!! если вызывается из CCommonGrammarInterpretation,
                                        //то != NULL но если из друго места (например, из CMultiWordCreator),
                                        //где нет грамматики, то == NULL

};
