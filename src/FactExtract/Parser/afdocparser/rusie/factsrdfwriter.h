#pragma once

#include <library/json/json_value.h>
#include <FactExtract/Parser/afdocparser/common/afdocbase.h>
#include <util/system/mutex.h>
#include "factsxmlwriter.h"
#include "leadgenerator.h"
#include "factscollection.h"


class CFactsRDFWriter: public CSimpleXMLWriter, protected CFactsWriterBase {

public:

    // Collect-mode constructor
    CFactsRDFWriter(const CParserOptions::COutputOptions& parserOutputOptions, Stroka path, ECharset encoding);

    // Stream-mode constructor
    //CFactsRDFWriter(const CParserOptions::COutputOptions& parserOutputOptions, TOutputStream* outStream, ECharset encoding);

    void AddFacts(const CTextProcessor& pText, const Stroka& url = Stroka(), const Stroka& siteUrl = Stroka(), const Stroka& strArtInLink = Stroka(), const SKWDumpOptions* pTask = NULL);
    void AddFactsToStream(TOutputStream& out_stream, const CTextProcessor& pText, const Stroka& url, const Stroka& siteUrl, const Stroka& strArtInLink, const SKWDumpOptions* pTask);

    void SetErrorStream(TOutputStream* errorStream);
    void PrintError(const Stroka& str);

private:

//    TXmlNodePtr WriteEqualFactsPair(int iFact1, int iFact2, EFactEntryEqualityType FactEntryEqualityType,
//                                   Stroka strFieldName1, Stroka strFieldName2);
//    void WriteEqualFactsPair(TXmlNodePtrBase& node, const CTextProcessor& pText,
//                             const SFactAddress& fact1, const SFactAddress& fact2,
//                             EFactEntryEqualityType equalityType, Stroka fieldName1, Stroka fieldName2);

    TXmlNodePtr WriteFact(const SFactAddress& factAddress, int iFactId, const CTextProcessor& pText, int& iStart, int& iEnd/*, CLeadGenerator& leadGenerator*/);

    void AddBoolAttribute(bool val, TXmlNodePtrBase piFact, const fact_field_descr_t& fieldDescr);

    void AddTextAttribute(const CTextProcessor& pText, const CTextWS* pFioWS, const Wtroka& strVal, TXmlNodePtrBase piFact, const CSentenceRusProcessor* pSent, const fact_field_descr_t& fieldDescr, const Stroka& strFactName, const SFactAddress& factAddress);

    //void AddFioAttributes(const CTextProcessor& pText, const CFioWS* pFioWS, TXmlNodePtrBase piFact, const fact_field_descr_t& fieldDescr, const Stroka& strFactName, const SFactAddress& factAddress);

    //void AddDateAttribute(const CTextProcessor& pText, const CDateWS* pDateWS, TXmlNodePtrBase piFact, const fact_field_descr_t& fieldDescr, const SFactAddress& factAddress);

    //TXmlNodePtr WriteFacts(const CTextProcessor& pText, CLeadGenerator& leadGenerator);
    //TXmlNodePtr WriteEqualFacts(const CTextProcessor& pText);
    //TXmlNodePtr WriteLeads(const CTextProcessor& pText, CLeadGenerator& leadGenerator);
    //TXmlNodePtr WriteFoundLinks(const CTextProcessor& pText, const SKWDumpOptions& task);

private:

    TOutputStream* m_pErrorStream;
    TMutex m_SaveFactsCS;
};
