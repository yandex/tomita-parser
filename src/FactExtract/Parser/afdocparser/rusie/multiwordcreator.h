#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/rus/word.h>
#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include <FactExtract/Parser/afdocparser/rus/namecluster.h>
#include <FactExtract/Parser/afdocparser/rus/wordvector.h>
#include "dictsholder.h"

class CReferenceSearcher;

class CFactWS;

class CMultiWordCreator
{
public:
    CMultiWordCreator(CWordVector& _Words, CWordSequence::TSharedVector& wordSequences);

    void CreateWordIndexes(const yset<SArtPointer>& artPointers, yvector<SWordHomonymNum>& WordIndexes, bool bAddEndOfStreamSymbol);

    bool FindKWWords(const SArtPointer& artP, EDicType dicType);
    void FindKWWords(const yset<SArtPointer>& kw_type);

    inline bool FindKWWords(TKeyWordType kw_type, EDicType dicType /*= KWDict*/) //возвращает false если уже искала раньше это KWWord
    {
        return FindKWWords(SArtPointer(kw_type), dicType);
    }

    inline bool FindKWWords(const Wtroka& strTitle, EDicType dicType /*= KWDict*/) //возвращает false если уже искала раньше этот strTitle
    {
        return FindKWWords(SArtPointer(strTitle), dicType);
    }

    void PutReferenceSearcher(CReferenceSearcher* pRefSearcher) { m_pReferenceSearcher = pRefSearcher; };
    CReferenceSearcher* GetReferenceSearcher() { return m_pReferenceSearcher; };
    yvector<SWordHomonymNum>* GetFoundWords(TKeyWordType type, EDicType dicType, bool bTryToFind = true);
    int GetFoundWordsCount(TKeyWordType type, EDicType dicType);
    yvector<SWordHomonymNum>* GetFoundWords(const Wtroka& strTitle, EDicType dicType, bool bTryToFind = true);
    yvector<SWordHomonymNum>* GetFoundWords(const SArtPointer& artP, EDicType dicType, bool bTryToFind = true);
    void FindKeyWordsAfterAnalyticFormBuilder(EDicType dicType);

    void FindWordsInPeriod(const SArtPointer& artP, const CWordsPair& wp);
    void GetFoundWordsInPeriod(const SArtPointer& artP, yvector<SWordHomonymNum>& foundWords, const CWordsPair& wp) const;

    void PrintWordsFeatures(TOutputStream &stream, ECharset encoding) const;        //for debugging

    //add and take onwership
    SWordHomonymNum TakeMultiWord(CWordSequence* ws, const TGramBitSet& newPos = TGramBitSet()) {
        return AddMultiWordInt(ws, true, newPos, CWordsPair());
    }

    //add without onwership
    SWordHomonymNum AddMultiWord(CWordSequence* ws, const TGramBitSet& newPos = TGramBitSet()) {
        return AddMultiWordInt(ws, false, newPos, CWordsPair());
    }

private:
    SWordHomonymNum AddMultiWordInt(CWordSequence* ws, bool takeOnwership, const TGramBitSet& newPos, const CWordsPair& searchAreaWP);

    void CollectArticleInfo(const SArtPointer& artP, EDicType dicType,
                            yvector<TGztArticle>& gztArticles, yvector<SDictIndex>& auxArticles, bool hasTextKeys) const;

    void FindAuxKWWords(yvector<SDictIndex>& indexes, EDicType dicType);
    void AddFIOMultiWord(CFioWordSequence& wordsPair, const TArticleRef& article, bool bWithoutSurname);
    void AddQuoteMultiWord(CWordSequence& ws, const TArticleRef& article);
    bool FindKeyWords(EDicType dicType);

    void ProcessTomitaAuxArticle(SDictIndex& dicIndex);
    void ProcessTomitaGztArticle(const TGztArticle& article);
    void ProcessTaggerGztArticle(const TGztArticle& article);

    void ProcessAlgArticle(const TArticleRef& article);
    void ProcessAlgArticle(const TArticleRef& article, const Stroka& alg);

    void FindCompanyNamesInQuotes(const TArticleRef& article);
    CWord* GetWordForMultiWord(CWordSequence& ws, Wtroka& str, SWordHomonymNum& newWH);

    void SubstituteByMultiWord(SWordHomonymNum& WordHomonymNum, yvector<SWordHomonymNum>& WordIndexes);
    void InitWordIndexes(yvector<SWordHomonymNum>& WordIndexes);
    void AddFoundArticle(TKeyWordType article_type, const Wtroka& article_title, SWordHomonymNum& word);
    void AddFoundArticle(TKeyWordType article_type, const Wtroka& article_title, const SWordHomonymNum& word, const CWordsPair& searchAreaWP);
    bool HasToAddGrammemsFromMainWord(const CWordSequence& ws);
    void NormalizeMultiWordHomonym(CWord* pW,  CHomonym* pH);
    void FindExtractedNamesInText(const TArticleRef& article, EArticleTextAlg eAlg);

private:
    yset<SArtPointer> m_FoundArticles;

    bool m_GztArticlesFound;
    bool m_TextKWFound[DICTYPE_COUNT]; //для какого словаря уже вызывали ф-цию FindKeyWords

    CReferenceSearcher* m_pReferenceSearcher;

    ymap<TKeyWordType, yvector<SWordHomonymNum> > m_KW2Words;
    ymap<Wtroka, yvector<SWordHomonymNum> > m_Title2Words;
    ymap<CWordsPair, yvector<SWordHomonymNum> > m_FoundWordsByPeriod;

public:
    CWordSequence::TSharedVector& m_wordSequences;
    CWordVector& m_Words;
    bool m_bProcessedByGrammar;
};
