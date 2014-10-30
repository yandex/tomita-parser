#pragma once

#include "doclistretrieverbase.h"
#include "docstreambase.h"
#include "agencyinforetriever.h"

#include <FactExtract/Parser/common/utilit.h>
#include <FactExtract/Parser/common/dateparser.h>

#include <util/generic/stroka.h>

////
// Docs Reader for MapReduce lenval format
////
class CStdinMapReduceDocsReader :
    public CDocListRetrieverBase,
    public CDocStreamBase,
    public CAgencyInfoRetriver
{
public:
    // Workaround for encoding: treat all stream documents as having same encoding,
    // which we know in advance. For most cases, UTF8 can be used safely, as several Yandex-people say.
    // When it is discovered to be wrong, we just should find the way to get encoding from the document itself.
    CStdinMapReduceDocsReader(ECharset encoding, Stroka subkey);
    virtual ~CStdinMapReduceDocsReader();


    /*CDocListRetrieverBase methods*/
    virtual bool GetNextDocInfo(Stroka& strFilePath, SDocumentAttribtes& attrs);

public:
    /*CDocStreamBase methods*/
    virtual bool  GetDocBodyByAdress(const Stroka& strAddr, CDocBodyBase& body);
    virtual bool  AddAddressHint(const Stroka& /*strAddr*/)
    {
        return false;
    };

protected:
    /*CDocStreamBase methods*/
    virtual bool ReadInBuffer(ui8* p_buffer);
    virtual long GetBufferLen();
    virtual int GetCharset();

protected:
    void ReadLV(TInputStream& is, Stroka& value);

protected:
    ECharset Encoding;
    ui32 DocLen;
    size_t CurrentDocNumber;
    Stroka Subkey;

public:
    size_t LastDocNumber;
};
