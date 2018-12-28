#pragma once

#include <util/generic/stroka.h>

#include <FactExtract/Parser/common/simplexmlwriter.h>
#include <FactExtract/Parser/afdocparser/common/afdocbase.h>
#include <util/system/mutex.h>
#include "leadgenerator.h"
#include "factscollection.h"
#include "parseroptions.h"


const char g_strValAttr[] = "val";
const char g_strInfoAttr[] = "info";
const char g_strGeoArtAttr[] = "gart";

class CFactsWriterBase
{
public:
    CFactsWriterBase(const CParserOptions::COutputOptions& parserOutputOptions, bool writeLeads = true, bool writeSimilarFacts = false)
        : IsWriteLeads(writeLeads)
        , IsWriteSimilarFacts(writeSimilarFacts)
        , WrittenSimilarFacts(0)
        , ParserOutputOptions(parserOutputOptions)
    {
    }

    Wtroka GetWords(const CTextWS& wordsPair) const;
    bool CheckFieldValue(const CWordsPair* wp, const fact_field_descr_t& fieldDescr, const Stroka& strFactName) const;

    bool GetGeoArts(const CTextProcessor& pText, const CTextWS* pTextWS, const CSentenceRusProcessor* pSent,
                    const Stroka& strFactName, Wtroka& sRes, const SFactAddress& factAddress) const;

    bool FindFactId(const SFactAddress& factAddr, int& id) const {
        TFactIdMap::const_iterator it = FactIdMap.find(factAddr);
        if (it == FactIdMap.end())
            return false;
        id = it->second;
        return true;
    }

    void ResetSimilarFacts() {
        WrittenSimilarFacts = 0;
    }

    bool CheckSimilarFactsLimit() {
        static const size_t MAX_SIMILAR_FACTS_FROM_CLUSTER = 1000;      // avoiding explosion
        if (WrittenSimilarFacts >= MAX_SIMILAR_FACTS_FROM_CLUSTER)
            return false;
        ++WrittenSimilarFacts;
        return true;
    }

private:
    void InsertArticle(const Wtroka& strDescr, const Wtroka& pArtTitle, TKeyWordType kwType, TKeyWordType kwMainType,
                       const CDictsHolder* pDictsHolder, bool bAnyGeoType, yset<Wtroka>& arts) const;

protected:
    bool IsWriteLeads;
    bool IsWriteSimilarFacts;

    typedef ymap<SFactAddress, int> TFactIdMap;
    TFactIdMap FactIdMap;

    size_t WrittenSimilarFacts;
    const CParserOptions::COutputOptions& ParserOutputOptions;
};

class CFactsXMLWriter: public CSimpleXMLWriter, protected CFactsWriterBase
{
public:
    CFactsXMLWriter(const CParserOptions::COutputOptions& parserOutputOptions, Stroka path, ECharset encoding);
    CFactsXMLWriter(const CParserOptions::COutputOptions& parserOutputOptions, TOutputStream* outStream, ECharset encoding);
    CFactsXMLWriter(const CParserOptions::COutputOptions& parserOutputOptions, Stroka path, ECharset encoding, bool bReload, bool bAppend = false);
    CFactsXMLWriter(const CParserOptions::COutputOptions& parserOutputOptions, TOutputStream* outStream, ECharset encoding, bool bReload, bool bAppend = false);

    void AddFacts(const CTextProcessor& pText, const Stroka& url, const Stroka& siteUrl = Stroka(),
                  const Stroka& strArtInLink = Stroka(), const SKWDumpOptions* pTask = NULL);
    void AddFactsToStream(TOutputStream& out_stream, const CTextProcessor& pText, const Stroka& url,
                          const Stroka& siteUrl, const Stroka& strArtInLink, const SKWDumpOptions* pTask);

    void SetErrorStream(TOutputStream* errorStream);
    void PrintError(const Stroka& str);
    void PutWriteLeads(bool b);
    void PutWriteSimilarFacts(bool b);

private:
    TXmlNodePtr WriteEqualFactsPair(int iFact1, int iFact2, EFactEntryEqualityType FactEntryEqualityType,
                                   Stroka strFieldName1, Stroka strFieldName2);
    void WriteEqualFactsPair(TXmlNodePtrBase& node, const CTextProcessor& pText,
                             const SFactAddress& fact1, const SFactAddress& fact2,
                             EFactEntryEqualityType equalityType, Stroka fieldName1, Stroka fieldName2);

    TXmlNodePtr WriteFact(const SFactAddress& factAddress, int iFactId, const CTextProcessor& pText, CLeadGenerator& leadGenerator);

    void AddBoolAttribute(bool val, TXmlNodePtrBase piFact, const fact_field_descr_t& fieldDescr);

    void AddTextAttribute(const CTextProcessor& pText, const CTextWS* pFioWS, const Wtroka& strVal, TXmlNodePtrBase piFact,
                          const CSentenceRusProcessor* pSent, const fact_field_descr_t& fieldDescr,
                          const Stroka& strFactName, const SFactAddress& factAddress);

    void AddFioAttributes(const CTextProcessor& pText, const CFioWS* pFioWS, TXmlNodePtrBase piFact, const fact_field_descr_t& fieldDescr, const Stroka& strFactName, const SFactAddress& factAddress);

    void AddDateAttribute(const CTextProcessor& pText, const CDateWS* pDateWS, TXmlNodePtrBase piFact, const fact_field_descr_t& fieldDescr, const SFactAddress& factAddress);

    TXmlNodePtr WriteFacts(const CTextProcessor& pText, CLeadGenerator& leadGenerator);
    TXmlNodePtr WriteEqualFacts(const CTextProcessor& pText);
    TXmlNodePtr WriteLeads(const CTextProcessor& pText, CLeadGenerator& leadGenerator);
    TXmlNodePtr WriteFoundLinks(const CTextProcessor& pText, const SKWDumpOptions& task);

private:
    TOutputStream* m_pErrorStream;
    TMutex m_SaveFactsCS;
};
