#pragma once

#include <util/generic/vector.h>
#include <util/generic/hash_set.h>

#include <FactExtract/Parser/afdocparser/rus/text.h>
#include "sentencerusprocessor.h"
#include "factscollection.h"
#include "parseroptions.h"


void UpdateFirstLastWord(int& iFirstWord, int& iLastWord, const CWordSequence* ws);

class CReferenceSearcher;
namespace NGzt {  class TCachingGazetteer; }

struct TSimpleFactPosition {
    int firstWord, lastWord, sentenceNo;
    int byteStart, byteEnd;

};

class CTextProcessor: public CText
{
public:
    CTextProcessor(const CParserOptions* parserOptions);
    virtual ~CTextProcessor();

    virtual void FreeData();

    void SerializeFacts(yvector<TPair<Stroka, ymap<Stroka, Wtroka> > >& out_facts, yvector<TPair<int, int> >& facts_positions);
    void GetFactsPositions(yvector<TSimpleFactPosition>* factsPositions);
    virtual void RunFragmentation();
    void RunFragmentationWithoutXml();

    using CText::analyzeSentences;
    virtual void analyzeSentences();
    virtual void SetCurSent(int iSent);

    void SetErrorStream(TOutputStream* pErrStream) { m_pErrorStream = pErrStream;   }
    void PrintError(const char* msg) { if (m_pErrorStream) (*m_pErrorStream) << msg << Endl; }
    void PrintError(const Stroka& msg, const yexception* error);

    inline CSentenceRusProcessor* GetSentenceProcessor(int i) {
        return CheckedCast<CSentenceRusProcessor*>(m_vecSentence[i]);
    };

    inline const CSentenceRusProcessor* GetSentenceProcessor(int i) const {
        return CheckedCast<const CSentenceRusProcessor*>(m_vecSentence[i]);
    }

    inline CSentence* GetSentence(size_t i) {
        return m_vecSentence[i];
    }

    inline const CSentence* GetSentence(size_t i) const {
        return m_vecSentence[i];
    }

    const CTime&    GetDateFromDateField() const;
    virtual CSentenceBase* CreateSentence();
    //void FindSituatiations();
    bool FindSituatiations(int iSent, const SArtPointer& artP);
    void GetFoundMWInLinks(yvector<SMWAddressWithLink>& mwFounds, const yset<SArtPointer>& artToFind) const;
    void GetFoundMWInLinks(yvector<SMWAddressWithLink>& mwFounds, const SArtPointer& artPointer) const;


    bool RunTagger(const TGztArticle& taggerArticle);
    virtual void SetParserOptions(const CParserOptions* parserOptions) {
        m_ParserOptions = parserOptions;
    };

    // run mw-finder on each sentence of the text
    void FindMultiWords(const SArtPointer& art);

    CFactFields& GetFact(const SFactAddress& factsAddr);
    const CFactFields& GetFact(const SFactAddress& factsAddr) const;
    void FillFactAddresses(yset<SFactAddress>& factAddresses);

    const CFactsCollection& GetFactsCollection() const {
        return m_FactsCollection;
    }

    CFactsCollection& GetFactsCollection() {
        return m_FactsCollection;
    }

    virtual bool IgnoreUpperCase() const;


    int m_iTouchedSentencesCount;
    const CParserOptions* m_ParserOptions;

protected:
    virtual size_t GetMaxNamesCount() {
        return m_ParserOptions->m_maxNamesCount;
    }

private:
    void CreateReferenceSearcher();
    bool ShouldRunFramentation(CSentence& sent);

    virtual bool HasToResolveContradictionsFios();

private:
    // for each document we cache found articles without loading same article multiple times
    THolder<NGzt::TCachingGazetteer> GztCache;
    THolder<CReferenceSearcher> m_pReferenceSearcher;

    TOutputStream* m_pErrorStream;
    CFactsCollection    m_FactsCollection;

    // an ids of tagger gzt articles already run on this text
    yhash_set<ui32> UsedTaggerIds;
};
