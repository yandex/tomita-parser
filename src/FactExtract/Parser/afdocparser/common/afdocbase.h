#pragma once

#include <util/generic/vector.h>
#include <util/generic/stroka.h>

#include "textbase.h"
#include <FactExtract/Parser/afdocparser/rus/namecluster.h>


enum ETypeOfDocument
{
    DocumentText = 2,
    DocumentHtml = 4,
};

class CParserOptions;

class CDocBase
{
public:
    CDocBase();
    virtual ~CDocBase();

    virtual void CreateTextClass(const CParserOptions *) = 0;

    //void SetDocLang(docLanguage lang);

    void AddDocLangAltInfo(docLanguage lang, bool bUseAsPrime, bool bUseAlways);
    void AddDocLangAltInfo(const typeLangAltInfo &langAltInfo);

    // DocumentHtml or DocumentText
    void SetDocType(ETypeOfDocument type);
    void SetDocNum(ui32 num);

// Proceed - create m_sentences vector
    bool Proceed(const TWtringBuf& src, const CParserOptions* parserOptions);
    bool Proceed(const TWtringBuf& src, const SDocumentAttribtes& docAttrs, const CParserOptions* parserOptions);
    bool ProceedOnlyMorph(const TWtringBuf& src, const SDocumentAttribtes& docAttrs);

// Processed plain text size
    int GetPlainTextSize();

    inline CTextBase* GetText() {
        return m_pText.Get();
    }

    inline const CTextBase* GetText() const {
        return m_pText.Get();
    }

// Note: two next functions use ONLY m_sentences

// Clear sentences vector [CTextBase::FreeExternOutput()]
// Set all docmaps numbers [m_nParaNum...m_nWordNum] to 0
    virtual void FreeData();
// Note: not affect allocated memory for docmaps
//  do nothing with OutWordStream
//  not affect existing internal CTextBase class

    // Remove LangAlt information
    void RemoveDocLangAltInfo() { m_vecLangAltInfo.clear(); }

// Data:
public:

    Stroka m_strDateFieldText;

protected:
    bool ProceedInternal(const TWtringBuf& src, const SDocumentAttribtes& docAttrs, const CParserOptions* parserOptions, bool nAnalyze);
    yvector<typeLangAltInfo> m_vecLangAltInfo;
    ETypeOfDocument m_dtyp;
    ui32 m_dnum;

    THolder<CTextBase> m_pText;
    Wtroka m_Source;

protected:
    int  m_iTagsSize;

// Code:
protected:
    void proceedText();
    int  findNextField(int from,size_t &fldNo);
    int  findNextFieldOptimized(int from,size_t &fldNo);
    int  calcDefaultFieldUid(size_t& iField);
    void proceedHtml();
    int  proceedHtmlHead(); // return pos just after <body ...>
    void proceedHtmlTitle(int beg,int end); // output <TITLE> as DefaultField

    bool isTagNoContent(const Stroka &tag) const;
    bool isTagParaBreak(const Stroka &tag) const;
    bool isTagSentBreak(const Stroka &tag) const;

    bool findHtmlTag(int from,int &beg,int &end,Stroka &tag);
    bool findHtmlTag(int from,const Stroka &tag,int &beg,int &end);

    void removeHtmlTag(int beg,int end,int brkType);
    bool refineHtmlStr(int beg,int end); // return string is non-empty

    bool isSpace(int i) { return(i==' ' || i=='\t' || i=='\r' || i=='\n'); }

// Error report
protected:
    TOutputStream* m_pErrorStream;

public:
    void SetErrorStream(TOutputStream* pErrorStream) { m_pErrorStream = pErrorStream; }
    void PrintError(const char* msg)
    {
        if (m_pErrorStream)
            (*m_pErrorStream) << msg << Endl;
    }

// Debug function(s)
public:
    Stroka getFieldString(int uid);
    bool isFieldData(int uid);
    bool isFieldText(int uid);
};

