#pragma once
#include <FactExtract/Parser/afdocparser/common/afdocsinglelanguage.h>
#include <FactExtract/Parser/afdocparser/rus/text.h>

class CDoc: public CDocSingleLanguage
{
protected:
    virtual CTextBase* CreateSingleTextClass(const CParserOptions* parserOptions) const {
        return new CText(parserOptions);
    }
};
