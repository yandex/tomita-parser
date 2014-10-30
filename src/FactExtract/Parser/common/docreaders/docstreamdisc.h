#pragma once

#include <util/system/oldfile.h>
#include <util/generic/stroka.h>

#include "docstreambase.h"
#include <FactExtract/Parser/common/commonparm.h>


class CDocStreamDisc : public CDocStreamBase
{
public:
    CDocStreamDisc(const CCommonParm& parm);
    virtual ~CDocStreamDisc();
    virtual bool  Init(const Stroka& str);
    virtual bool  AddAddressHint(const Stroka& strAddr);
    virtual bool  GetDocBodyByAdress(const Stroka& strAddr, CDocBodyBase& body);

protected:
    virtual bool ReadInBuffer(ui8* p_buffer);
    virtual long GetBufferLen();
    virtual int     GetCharset();

protected:
    ECharset Encoding;

    TOldOsFile m_CurFile;
    Stroka m_CurFilePath;
    static int s_BufSize;
    const CCommonParm & m_Parm;

};
