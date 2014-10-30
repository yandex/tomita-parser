#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "sentencerusprocessor.h"
#include <FactExtract/Parser/common/sdocattributes.h>

namespace NGzt {  class TCachingGazetteer; }

class CTextProcessor;


const int g_iLeftReferenceSearcherBorder = 10;
typedef ymap<long, yset< TPair<int,int> > > _text_index;
typedef ymap<long, yset< TPair<int,int> > >::iterator _text_ind_it;
typedef ymap<long, yset< TPair<int,int> > >::const_iterator _text_ind_const_it;

typedef ymap< int, yvector<CWordSequence*> > artificial_comp_names;
typedef ymap< int, yvector<CWordSequence*> >::iterator artificial_names_it;
typedef ymap< int, yvector<CWordSequence*> >::const_iterator const_art_comp_names_it;

struct SNonGrammarNames
{
    yset<EArticleTextAlg> m_ProcessedTextAlgs;
    artificial_comp_names m_DocArtificialExtractedFacts; //ключ - номер предложения; хранит вектор искусственно (неграмматически) выделенных из текста имен компаний
};

class CReferenceSearcher
{
    friend class CExtractedFieldSearcher;
    friend class CAnaphoraResolution;

public:
    CReferenceSearcher(NGzt::TCachingGazetteer& gazetteer, CTextProcessor& textProcessor, CTime& dateFromDateFiled);

    void SetCurSentence(int iSent) { m_iCurSentence = iSent; };
    int GetCurSentence() const { return m_iCurSentence; };

    const CTime& GetDocDate() const { return m_dateFromDateFiled; };
    Wtroka GetSentenceString(int iSentNum);
    void BuildTextIndex();
    long GetThreeLetterIndex(const Wtroka& s_lem) const;
    CSentenceRusProcessor* GetSentenceProcessor(int i) const;

protected:
    //void StartCompanyNameGrammar();
    bool IsEqualCompName(const CWordSequenceWithAntecedent& CompNameF, int iSentNum);

    //начало: интерфейс доступа к m_DocNonGrammarFacts
    void PushArtificialFacts(int iSentNum, CFactsWS* pFactWS);
    void GetArtificialFacts(int iSentNum, yvector<CWordSequence*>& rFoundFPCs);
    void SetArtificialFactProcessed(EArticleTextAlg eTextAlg);
    bool IsArtificialFactProcessed(EArticleTextAlg eTextAlg) const;
    //конец: интерфейс доступа к m_DocNonGrammarFacts

public:
    NGzt::TCachingGazetteer& GztCache;

    CTextProcessor& TextProcessor;
    yvector<CSentence*>& m_vecSentence;     // TODO: use TextProcessor.m_vecSentence directly

    _text_index m_TextIndex;
    SDocumentAttribtes m_DocumentAttribtes;

private:
    const CTime&                m_dateFromDateFiled;
    int                         m_iCurSentence;
    //выделенные без грамматик компании
    SNonGrammarNames            m_DocNonGrammarFacts;
};
