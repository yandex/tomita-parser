#pragma once

#include "doclistretrieverbase.h"
#include "docstreambase.h"

#include <FactExtract/Parser/common/utilit.h>
#include <FactExtract/Parser/common/dateparser.h>

#include <util/generic/stroka.h>

class COneDocReader :
    public CDocListRetrieverBase,
    public CDocStreamBase
{
public:
    COneDocReader(TInputStream& stream, ECharset encoding);
    virtual ~COneDocReader();

public:
    /*CDocListRetrieverBase methods*/
    virtual bool GetNextDocInfo(Stroka& strFilePath, SDocumentAttribtes& attrs);

public:
    /*CDocStreamBase methods*/
    virtual bool  GetDocBodyByAdress(const Stroka& strAddr, CDocBodyBase& body);
    virtual bool  AddAddressHint(const Stroka& /*strAddr*/)
    {
        return false;
    };

    yset<int> m_setDocNums;
    time_t  m_Date;

protected:
    /*CDocStreamBase methods*/
    virtual bool ReadInBuffer(ui8* p_buffer);
    virtual long GetBufferLen();
    virtual int GetCharset();

    ECharset Encoding;
    TInputStream& file;

    Stroka m_CurrentLine;
    int m_CurrentLineNumber;

    size_t CurrentDocNumber;

    bool m_bDone;

public:
    size_t LastDocNumber;
};
