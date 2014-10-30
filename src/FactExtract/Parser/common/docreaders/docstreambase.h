#pragma once

#include <util/generic/stroka.h>

#include "docbodybase.h"


class CDocStreamBase
{
    friend class CDocBodyBase;
public:
    CDocStreamBase() {};
    virtual ~CDocStreamBase() {};

public:
    virtual bool  AddAddressHint(const Stroka& strAddr) = 0;
    virtual bool  GetDocBodyByAdress(const Stroka& strAddr, CDocBodyBase& body) = 0;

protected:
    virtual bool ReadInBuffer(ui8* p_buffer) = 0;
    virtual long GetBufferLen() = 0;
    virtual int     GetCharset() = 0;

    CDocStreamBase(CDocStreamBase&);
    CDocStreamBase& operator=(CDocStreamBase&);

};
