#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "referencesearcher.h"

struct SFieldToSent
{
    SFieldToSent(int iSent, const CTextWS* pExtField)
    {
        m_iSent = iSent;
        m_pExtField = pExtField;
    };

    int m_iSent;
    const CTextWS* m_pExtField;
};

typedef ymap< int, yvector< yset<long> > > artificial_val_index;
typedef ymap< int, yvector< yset<long> > >::const_iterator const_val_ind_it;

class CExtractedFieldSearcher
{
public:
    CExtractedFieldSearcher(CReferenceSearcher* pReferenceSearcher, EArticleTextAlg eAlg)
        : m_pReferenceSearcher(pReferenceSearcher)
        , m_eArtAlg(eAlg)
    {
    }

    void SearchExtractedNamesInText(int iSentNum, yvector<CWordSequence*>& rFoundFPCs);

protected:
    void StartGrammarsContainingExtractedField();
    void BuildCompaniesIndex();
    bool IsEqualExtNameIndex(int iCompF, int iCompS) const;
    bool IsEqualExtNameWords(int iCompF, int iCompS) const;
    void CollectUniqueNamesFromText();
    void LoadGrammarExtractedFacts();
    void ProvideExtNameSearchByDocIndex();
    void CompareCandidatesWithExtractedPatterns(yset< TPair<int, int> >& rArtExtCandidates);
    void CompareCandidatesWithExtractedPatternWordByWord(yset< TPair<int, int> >& rArtExtCandidates, int iPatternID);
    void BuildArtificialCandidates(yset< TPair<int, int> >& rArtExtCandidates, int iPatternID);
    void PushArtificialFacts(int iSentNum, CFactsWS* pFactWS);
    bool IsEqualWords(const CWord& rWPatt, const CWord& rWCand, const SArtPointer& art) const;
    Stroka GetExtractedFactName() const;
    Stroka GetExtractedFieldName() const;

protected:
    CReferenceSearcher* m_pReferenceSearcher;

    yvector<SFieldToSent> m_DocGramExtVals;//значения поля, выделенного различными грамматиками
    artificial_val_index m_FirstWrdGramExtValIndex; //трехбуквенный индекс, построенный для каждого из выделенных значений поля
    yvector<int>             m_UniqueGramExtVals; //уникальные значения поля, поиск и выделение которых осуществляется в тексте без использования грамматик
                                             //вектор m_UniqueGramExtVals содержит номера элементов из массива m_DocGramExtVals
    EArticleTextAlg         m_eArtAlg; //СОСТАВ = alg:comp_in_text - вызов алгоритма из aux_dic статьи
};
