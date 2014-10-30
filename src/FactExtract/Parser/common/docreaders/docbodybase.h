#pragma once

#include <util/system/defaults.h>
#include <util/generic/stroka.h>
#include <util/generic/strbuf.h>
#include <util/charset/doccodes.h>

class CDocStreamBase;

class CDocBodyBase
{
    friend class CDocStreamBase;
public:
    CDocBodyBase();
    virtual ~CDocBodyBase();
    const Wtroka& GetBody() const;
    bool FillBody(CDocStreamBase&, ECharset encoding);
    int GetCharset() const;
    void PutCharset(int);
    virtual void Reset();

    virtual void PutUnloadDocCheckSum(ui32 /*unloadDocCheckSum*/) {
    }

    virtual void Put_ltm(Stroka ltm){
        (void)ltm;
    }

    virtual void AddUrlAreaItem(const Stroka& /*strItem*/) {
    }

protected:
    Stroka m_TmpBuffer;
    Wtroka m_Body;
/*
    wchar16* m_pBody;
    size_t m_iBodyLen;
    size_t m_iBodyCapacity;
*/
    int m_iCharset;

    static int s_MemID;

};
