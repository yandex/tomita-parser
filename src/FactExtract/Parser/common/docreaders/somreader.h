#pragma once

#include "doclistretrieverbase.h"
#include "docstreambase.h"

#include <FactExtract/Parser/common/utilit.h>
#include <FactExtract/Parser/common/dateparser.h>
#include <FactExtract/Parser/common/libxmlutil.h>

#include <util/generic/stroka.h>

class CSomReader :
    public CDocListRetrieverBase,
    public CDocStreamBase
{

struct SSomDoc
{
    Stroka Url;
    Stroka Text;
};

public:
    CSomReader();
    virtual ~CSomReader();
    void Init(const Stroka& fileName );

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

    int CurrentDocNumber;

    yvector<SSomDoc> Docs;
};
