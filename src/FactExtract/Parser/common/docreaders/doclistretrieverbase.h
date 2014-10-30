#pragma once

#include "sourcedocinfo.h"

#include <FactExtract/Parser/common/sdocattributes.h>

#include <util/generic/stroka.h>

class CDocListRetrieverBase
{

public:

    typedef long PrefetchPos;

    CDocListRetrieverBase() {};
    virtual ~CDocListRetrieverBase() {};

public:
    //virtual bool Init(const Stroka& strParm) = 0;
    virtual bool GetNextDocInfo(Stroka& strFilePath, SDocumentAttribtes& attrs) = 0;
    virtual PrefetchPos FirstPos()    { return 0; };
    virtual PrefetchPos CurPos()    { return 0; };
    virtual bool Prefetch(SSourceDocInfo& /*sourceDocInfo*/, PrefetchPos /*Pos*/, PrefetchPos /*newPos*/) {
        return false;
    };
    virtual void Close() {};

protected:
    CDocListRetrieverBase(CDocListRetrieverBase&);
    CDocListRetrieverBase& operator=(CDocListRetrieverBase&);

};
