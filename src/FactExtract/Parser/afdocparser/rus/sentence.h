#pragma once

#include <util/generic/stroka.h>
#include <util/generic/vector.h>

#include <FactExtract/Parser/afdocparser/common/sentencebase.h>
#include "word.h"
#include <FactExtract/Parser/afdocparser/common/wordspair.h>
#include <FactExtract/Parser/afdocparser/common/graminfo.h>
#include "namecluster.h"
#include "wordvector.h"
#include "numberprocessor.h"
//////////////////////////////////////////////////////////////////////////////


enum ESentMarkUpType {LinkRef, SentMarkUpTypeCount};

struct SLinkRefMarkUp
{
    int m_iBeg;
    int m_iEnd;
    bool operator< (const SLinkRefMarkUp& x) const
    {
        if (m_iBeg == x.m_iBeg)
            return m_iEnd < x.m_iEnd;
        else
            return m_iBeg < x.m_iBeg;
    }
    Stroka m_strRef;
    ESentMarkUpType m_Type;
};

struct SSentMarkUp : public CWordsPair
{
    Stroka m_strInfo;
    ESentMarkUpType m_Type;
};

struct SUrl2Article
{
    Stroka m_strUrl;
    SDictIndex m_DicIndex;
};

class CSentence : public CSentenceBase
{
    friend class CText;
    friend class CFragmentationRunner;
public:

    virtual ~CSentence(void);

// Data
public:
    yvector<TSharedPtr<CWord> > m_words;
    CWordVector m_Words;

protected:
    CWordSequence::TSharedVector m_wordSequences;
    yvector<CNumber> m_Numbers;
    yset<SSentMarkUp> m_SentMarkUps;

// Code
public:
    virtual size_t getWordsCount() const{ return m_words.size(); }
    virtual CWordBase* getWord(size_t iNum) const;
    CWord* getWordRus(size_t iNum) const;
    const CWord& GetWord(const SWordHomonymNum& wh) const;
    const CHomonym& GetHomonym(const SWordHomonymNum& wh) const;
    virtual void   FreeData();
    virtual Wtroka BuildDates();
    virtual void BuildNumbers();
    virtual void   AddWord(CWordBase*);
    //virtual CWord* createWord(const CPrimGroup &group, const Wtroka& txt);
    void AddFIO(yset<Wtroka>& fioStrings, const CFIOOccurenceInText& wordsPairInText, bool bIndexed);
    void AddFIOWS(const CFIOOccurence& fioOccurence, const SFullFIO& fio, int iSimilarOccurencesCount);

    virtual void FindNames(yvector<CFioWordSequence>& foundNames, yvector<int>& potentialSurnames);

    bool CheckAllAreNames();

    CWordSequence::TSharedVector& GetWordSequences() {
        return m_wordSequences;
    };

    //CRusGramTab& GetAgramtab();
    const CWordVector& GetRusWords() const { return m_Words; }
    virtual void   AddNumber(CNumber& number);
    bool AddMarkUp(const SLinkRefMarkUp& LinkRefMarkUp);
    bool GetWSLemmaString(Wtroka& sLemmas, const CWordSequence& ws, bool bLem = true) const;

//main processor methods
protected:
    void InitNameTypes(bool bForSearching = false);
    void TryToPredictPatronymic(int iW, bool bForSearching = false);
    virtual Wtroka GetCapitalizedLemma(const CWord& w, int iH, Wtroka strLemma);
    void DelAdjShortAdverbHomonym();

    // add fio ws or delete.
    void TakeFioWS(TIntrusivePtr<CFioWordSequence> pFioWS);

public:
    void ToString(Wtroka& str) const;
    Wtroka ToString() const;
    Wtroka ToString(const CWordsPair& wp) const;
    Stroka ToHTMLColorString(const yvector<CWordsPair>& wp, const Stroka& sColor, ECharset encoding) const;

};

