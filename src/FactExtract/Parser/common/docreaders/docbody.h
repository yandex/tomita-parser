#pragma once

#include "docbodybase.h"

class CDocStreamBase;

class CDocBody : public CDocBodyBase
{
    friend class CDocStreamBase;
public:
    CDocBody();
    virtual ~CDocBody();
    /*bool OemToChar();*/        // win-only and never used
    //bool ConvertToHTML(CString& strConverter);
    virtual void Reset();
    ui32  GetUnloadDocCheckSum() const;

    virtual void PutUnloadDocCheckSum(ui32 unloadDocCheckSum);

protected:
    ui32  m_unloadDocCheckSum;

};
