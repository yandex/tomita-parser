#pragma once

#include <util/generic/cast.h>

#include <FactExtract/Parser/afdocparser/common/afdocsinglelanguage.h>
#include <FactExtract/Parser/afdocparser/rusie/textprocessor.h>

class CDocIE: public CDocSingleLanguage
{
public:
    CDocIE(docLanguage lang);

    const CTextProcessor* GetTextProcessor() const {
        return CheckedCast<const CTextProcessor*>(GetText());
    }

    CTextProcessor* GetTextProcessor() {
        return CheckedCast<CTextProcessor*>(GetText());
    }

protected:
    virtual CTextBase* CreateSingleTextClass(const CParserOptions* parserOptions) const;
};
