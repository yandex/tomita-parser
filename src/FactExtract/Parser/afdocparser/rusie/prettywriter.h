#pragma once

#include <util/generic/stroka.h>

#include <FactExtract/Parser/afdocparser/rusie/textprocessor.h>
#include <FactExtract/Parser/common/libxmlutil.h>

class CPrettyWriter
{
public:
    CPrettyWriter(Stroka strXSL, ECharset encoding, const CParserOptions& parserOptions);
    virtual ~CPrettyWriter(void) {
    }
    void AddDocument(CTextProcessor* pDoc, Stroka strUrl);
    void SaveToFile(const Stroka& name);

    void Reset();

protected:
    virtual TXmlNodePtrBase AddSentence(CSentenceRusProcessor* pSent, TXmlNodePtrBase piPar, int iSent);
    Stroka GetFioWords(CFioWS& wordsPair);
    virtual void AddInfo(CTextProcessor* pText, Stroka strUrl);
    void AddWS(CWordSequence* pWS, CSentence* pSent, int iSent);

    void AddWordAttribute(TXmlNodePtrBase piWord, int  iW, CSentence* pSent);
    Stroka GetWordStrAttr(int  iW, CSentence* pSent);
    Stroka GetWords(CWordSequence& wordsPair, CSentence* pSent);
    TXmlNodePtrBase GetFactXMLSubTree(const CFactFields& fact);
    void AddFactTable(const CFactFields& fact, int iSent, Stroka strUrl);
    void AddCellValue(TXmlNodePtrBase piCell, Wtroka strVal, Stroka strRef);
    void AddVisitedPagesTable();

    Stroka EncodeText(const Wtroka& text) {
        return WideToChar(text, Encoding);
    }

protected:
    ECharset Encoding;
    Stroka XmlStyleSheet;

    TXmlDocPtr m_piDoc;
    TXmlNodePtrBase m_piText;
    TXmlNodePtr m_piFPOs;
    TXmlNodePtrBase m_piAfDoc;
    ymap<Stroka, TXmlNodePtrBase> m_FactNameToXML;
    int m_iPrevSentCount;
    yset<Stroka> m_Urls;
    const CParserOptions& ParserOptions;
};
