#pragma once

#include <util/generic/stroka.h>
#include <util/system/mutex.h>
#include <util/stream/output.h>

#include <FactExtract/Parser/afdocparser/rusie/textprocessor.h>

#include "parseroptions.h"


class CAfDocPlainTextWriter
{
public:
    bool m_bPrintFileDelimiter;

    CAfDocPlainTextWriter(const Stroka& strFileName, ECharset encoding, const CParserOptions::COutputOptions& OutputOptions);
    void AddDocument(CTextProcessor* pDoc);
    void SetFileName(const Stroka& s);
    void SetEncoding(ECharset encoding) {
        YASSERT(encoding != CODES_UNKNOWN);
        Encoding = encoding;
    }
    void SetAppend(bool bAppend);
    Stroka AddSigns(CFioWordSequence* pFioWS, CSentence* _pSent);

protected:
    ECharset Encoding;
    Stroka m_strFileName;
    const CParserOptions::COutputOptions OutputOptions;
    bool m_bAppend;

    Stroka EncodeText(const Wtroka& text) const {
        return NStr::Encode(text, Encoding);
    }

    Stroka GetWords(CWordSequence& wordsPair, CSentence* pSent);
    void AddDocument(CTextProcessor* pText, TOutputStream* out);
    Stroka AddWS(CWordSequence* pWS, CSentence* pSent);
};
